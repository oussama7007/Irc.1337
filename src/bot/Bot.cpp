#include "Bot.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

static const std::size_t MAX_IRC_MESSAGE_SIZE = 512;
static const std::size_t MAX_RECEIVE_BUFFER_SIZE = 8192;
static const std::size_t RECEIVE_CHUNK_SIZE = 4096;
static const char *BOT_NICKNAME = "ft_bot";

//oadouz
static void closeBotSocket(int fd)
{
    if (fd >= 0 && close(fd) < 0)
    {
        std::cerr << "Warning: bot close() failed: "
                  << std::strerror(errno) << "." << std::endl;
    }
}

//oadouz
Bot::Bot(const std::string &host, int port, const std::string &password,
         const std::string &channel)
    : _host(host),
      _port(port),
      _password(password),
      _channel(channel),
      _socketFd(-1),
      _receiveBuffer()
{
    if (_host.empty() || _password.empty() || _channel.empty())
        throw std::invalid_argument("bot arguments must not be empty");
    if (_port < 1 || _port > 65535)
        throw std::invalid_argument("bot port must be between 1 and 65535");
}

//oadouz
Bot::~Bot()
{
    if (_socketFd >= 0)
    {
        closeBotSocket(_socketFd);
        _socketFd = -1;
    }
}

//oadouz
void Bot::connectToServer()
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    std::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream portStream;
    portStream << _port;

    int result = getaddrinfo(_host.c_str(), portStream.str().c_str(),
                             &hints, &addresses);
    if (result != 0)
        throw std::runtime_error(std::string("getaddrinfo() failed: ")
                                 + gai_strerror(result));

    for (struct addrinfo *address = addresses; address != NULL;
         address = address->ai_next)
    {
        int candidateFd = socket(address->ai_family, address->ai_socktype,
                                 address->ai_protocol);
        if (candidateFd < 0)
            continue;

        if (connect(candidateFd, address->ai_addr, address->ai_addrlen) == 0)
        {
            _socketFd = candidateFd;
            break;
        }
        closeBotSocket(candidateFd);
    }

    freeaddrinfo(addresses);

    if (_socketFd < 0)
        throw std::runtime_error("bot could not connect to the IRC server");
}

//oadouz
void Bot::sendLine(const std::string &line)
{
    if (line.empty() || line.size() + 2 > MAX_IRC_MESSAGE_SIZE)
        throw std::runtime_error("bot refused an invalid IRC line");

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\0' || line[i] == '\r' || line[i] == '\n')
            throw std::runtime_error("bot refused an IRC control byte");
    }

    std::string message = line + "\r\n";
    std::size_t sentBytes = 0;

    while (sentBytes < message.size())
    {
        ssize_t result = send(_socketFd, message.c_str() + sentBytes,
                              message.size() - sentBytes, 0);
        if (result < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error(std::string("bot send() failed: ")
                                     + std::strerror(errno));
        }
        if (result == 0)
            throw std::runtime_error("bot send() returned zero bytes");

        sentBytes += static_cast<std::size_t>(result);
    }
}

//oadouz
bool Bot::extractLine(std::string &line)
{
    line.clear();
    std::size_t newlinePosition = _receiveBuffer.find('\n');

    if (newlinePosition == std::string::npos)
    {
        if (_receiveBuffer.size() >= MAX_IRC_MESSAGE_SIZE)
            throw std::runtime_error("bot received an oversized IRC line");
        return false;
    }

    if (newlinePosition + 1 > MAX_IRC_MESSAGE_SIZE)
        throw std::runtime_error("bot received an oversized IRC line");

    line = _receiveBuffer.substr(0, newlinePosition);
    _receiveBuffer.erase(0, newlinePosition + 1);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\0' || line[i] == '\r')
            throw std::runtime_error("bot received an IRC control byte");
    }
    return true;
}

//oadouz
void Bot::greetJoinedUser(const std::string &line)
{
    if (line.empty() || line[0] != ':')
        return;

    std::size_t joinPosition = line.find(" JOIN ");
    std::size_t nicknameEnd = line.find('!');

    if (joinPosition == std::string::npos
        || nicknameEnd == std::string::npos
        || nicknameEnd > joinPosition)
        return;

    std::string nickname = line.substr(1, nicknameEnd - 1);
    if (nickname.empty() || nickname == BOT_NICKNAME)
        return;

    std::string joinedChannel = line.substr(joinPosition + 6);
    if (!joinedChannel.empty() && joinedChannel[0] == ':')
        joinedChannel.erase(0, 1);
    if (joinedChannel != _channel)
        return;

    sendLine("PRIVMSG " + _channel + " :Hello @" + nickname);
}

//oadouz
void Bot::processLine(const std::string &line)
{
    if (line.compare(0, 5, "PING ") == 0)
    {
        sendLine("PONG " + line.substr(5));
        return;
    }

    if (line.find(" 001 ") != std::string::npos)
    {
        sendLine("JOIN " + _channel);
        return;
    }

    if (line.find(" 433 ") != std::string::npos)
        throw std::runtime_error("bot nickname is already in use");
    if (line.find(" 464 ") != std::string::npos)
        throw std::runtime_error("bot password was rejected");

    greetJoinedUser(line);
}

//oadouz
void Bot::receiveMessages()
{
    char chunk[RECEIVE_CHUNK_SIZE];

    while (true)
    {
        ssize_t result = recv(_socketFd, chunk, sizeof(chunk), 0);

        if (result == 0)
            return;
        if (result < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error(std::string("bot recv() failed: ")
                                     + std::strerror(errno));
        }

        std::size_t receivedBytes = static_cast<std::size_t>(result);
        if (_receiveBuffer.size() > MAX_RECEIVE_BUFFER_SIZE
            || receivedBytes > MAX_RECEIVE_BUFFER_SIZE - _receiveBuffer.size())
            throw std::runtime_error("bot receive buffer limit exceeded");

        _receiveBuffer.append(chunk, receivedBytes);

        std::string line;
        while (extractLine(line))
            processLine(line);
    }
}

//oadouz
void Bot::run()
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        throw std::runtime_error(std::string("bot signal(SIGPIPE) failed: ")
                                 + std::strerror(errno));

    connectToServer();
    sendLine("PASS :" + _password);
    sendLine(std::string("NICK ") + BOT_NICKNAME);
    sendLine("USER ftbot 0 * :ft_irc bonus bot");

    std::cout << "[bot] Connected as " << BOT_NICKNAME
              << " and waiting for users in " << _channel << "." << std::endl;

    receiveMessages();
}
