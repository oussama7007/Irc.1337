#include "Bot.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

// The bot accepts several complete IRC messages in one TCP read, but a server
// cannot grow its receive state forever without a newline.
static const std::size_t MAX_BOT_RECEIVE_BUFFER = 8192;
static const std::size_t MAX_BOT_SEND_BUFFER = 65536;
static const std::size_t MAX_IRC_MESSAGE_SIZE = 512;

// A short bot reply leaves room for the server-added nick!user@host prefix.
static const std::size_t MAX_BOT_REPLY_SIZE = 300;
static const int CONNECTION_TIMEOUT_MS = 5000;
static const int BOT_POLL_TIMEOUT_MS = 1000;
static const std::size_t RECEIVE_CHUNK_SIZE = 4096;

static volatile sig_atomic_t g_botShouldStop = 0;

//oadouz
static std::string toIrcLowerCase(const std::string &text)
{
    std::string result = text;

    for (std::size_t i = 0; i < result.size(); ++i)
    {
        unsigned char character = static_cast<unsigned char>(result[i]);

        if (character >= 'A' && character <= 'Z')
            result[i] = static_cast<char>(std::tolower(character));
        else if (character == '[')
            result[i] = '{';
        else if (character == ']')
            result[i] = '}';
        else if (character == '\\')
            result[i] = '|';
        else if (character == '~')
            result[i] = '^';
    }
    return result;
}

//oadouz
static void handleBotShutdownSignal(int signalNumber)
{
    (void)signalNumber;
    g_botShouldStop = 1;
}

//oadouz
static void closeBotSocket(int fd)
{
    if (fd < 0)
        return;

    if (close(fd) < 0)
    {
        std::cerr << "Warning: bot close() failed for fd=" << fd << ": "
                  << std::strerror(errno) << "." << std::endl;
    }
}

//oadouz
static void setupBotSignals()
{
    struct sigaction ignoreAction;
    struct sigaction stopAction;

    std::memset(&ignoreAction, 0, sizeof(ignoreAction));
    std::memset(&stopAction, 0, sizeof(stopAction));

    ignoreAction.sa_handler = SIG_IGN;
    if (sigemptyset(&ignoreAction.sa_mask) < 0)
        throw std::runtime_error(std::string("bot sigemptyset(SIGPIPE) failed: ")
                                 + std::strerror(errno));
    if (sigaction(SIGPIPE, &ignoreAction, NULL) < 0)
        throw std::runtime_error(std::string("bot sigaction(SIGPIPE) failed: ")
                                 + std::strerror(errno));

    stopAction.sa_handler = handleBotShutdownSignal;
    if (sigemptyset(&stopAction.sa_mask) < 0)
        throw std::runtime_error(std::string("bot sigemptyset(shutdown) failed: ")
                                 + std::strerror(errno));
    if (sigaction(SIGINT, &stopAction, NULL) < 0)
        throw std::runtime_error(std::string("bot sigaction(SIGINT) failed: ")
                                 + std::strerror(errno));
    if (sigaction(SIGTERM, &stopAction, NULL) < 0)
        throw std::runtime_error(std::string("bot sigaction(SIGTERM) failed: ")
                                 + std::strerror(errno));
}

//oadouz
Bot::Bot(const std::string &host, int port, const std::string &password,
         const std::string &channel, const std::string &nickname)
    : _host(host),
      _port(port),
      _password(password),
      _channel(channel),
      _nickname(nickname),
      _socketFd(-1),
      _joinSent(false),
      _receiveBuffer(),
      _sendBuffer()
{
    if (_host.empty())
        throw std::invalid_argument("bot host must not be empty");
    if (_port < 1 || _port > 65535)
        throw std::invalid_argument("bot port must be between 1 and 65535");
    if (_password.empty())
        throw std::invalid_argument("bot password must not be empty");
    if (_channel.empty())
        throw std::invalid_argument("bot channel must not be empty");
    if (_nickname.empty())
        throw std::invalid_argument("bot nickname must not be empty");
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

    int addressResult = getaddrinfo(_host.c_str(), portStream.str().c_str(),
                                    &hints, &addresses);
    if (addressResult != 0)
        throw std::runtime_error(std::string("getaddrinfo() failed: ")
                                 + gai_strerror(addressResult));

    for (struct addrinfo *address = addresses; address != NULL;
         address = address->ai_next)
    {
        int candidateFd = socket(address->ai_family, address->ai_socktype,
                                 address->ai_protocol);
        if (candidateFd < 0)
            continue;

        if (fcntl(candidateFd, F_SETFL, O_NONBLOCK) < 0)
        {
            closeBotSocket(candidateFd);
            continue;
        }

        int connectResult = connect(candidateFd, address->ai_addr,
                                    address->ai_addrlen);
        if (connectResult < 0 && errno != EINPROGRESS)
        {
            closeBotSocket(candidateFd);
            continue;
        }

        if (connectResult < 0)
        {
            struct pollfd connectPollFd;
            connectPollFd.fd = candidateFd;
            connectPollFd.events = POLLOUT;
            connectPollFd.revents = 0;

            int readyCount = poll(&connectPollFd, 1, CONNECTION_TIMEOUT_MS);
            while (readyCount < 0 && errno == EINTR && !g_botShouldStop)
                readyCount = poll(&connectPollFd, 1, CONNECTION_TIMEOUT_MS);

            int socketError = 0;
            socklen_t errorLength = sizeof(socketError);

            if (readyCount <= 0
                || getsockopt(candidateFd, SOL_SOCKET, SO_ERROR,
                              &socketError, &errorLength) < 0
                || socketError != 0)
            {
                closeBotSocket(candidateFd);
                continue;
            }
        }

        _socketFd = candidateFd;
        break;
    }

    freeaddrinfo(addresses);

    if (_socketFd < 0)
        throw std::runtime_error("bot could not connect to any resolved address");
}

//oadouz
void Bot::queueRegistration()
{
    queueLine("PASS :" + _password);
    queueLine("NICK " + _nickname);
    queueLine("USER ftbot 0 * :ft_irc bonus bot");
}

//oadouz
void Bot::queueLine(const std::string &line)
{
    if (line.empty() || line.size() + 2 > MAX_IRC_MESSAGE_SIZE)
        throw std::runtime_error("bot refused an invalid outbound IRC line");

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\0' || line[i] == '\r' || line[i] == '\n')
            throw std::runtime_error("bot refused an outbound control byte");
    }

    if (_sendBuffer.size() > MAX_BOT_SEND_BUFFER
        || line.size() + 2 > MAX_BOT_SEND_BUFFER - _sendBuffer.size())
        throw std::runtime_error("bot send buffer limit exceeded");

    _sendBuffer += line;
    _sendBuffer += "\r\n";
}

//oadouz
void Bot::flushOutput()
{
    if (_sendBuffer.empty())
        return;

    ssize_t bytesSent = send(_socketFd, _sendBuffer.c_str(),
                             _sendBuffer.size(), 0);
    if (bytesSent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return;
        throw std::runtime_error(std::string("bot send() failed: ")
                                 + std::strerror(errno));
    }
    if (bytesSent == 0)
        throw std::runtime_error("bot send() returned zero bytes");

    std::size_t sentSize = static_cast<std::size_t>(bytesSent);
    if (sentSize > _sendBuffer.size())
        throw std::runtime_error("bot send-buffer accounting became invalid");
    _sendBuffer.erase(0, sentSize);
}

//oadouz
void Bot::receiveInput()
{
    char chunk[RECEIVE_CHUNK_SIZE];
    ssize_t bytesRead = recv(_socketFd, chunk, sizeof(chunk), 0);

    if (bytesRead == 0)
    {
        std::cout << "[bot] IRC server closed the connection." << std::endl;
        g_botShouldStop = 1;
        return;
    }
    if (bytesRead < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return;
        throw std::runtime_error(std::string("bot recv() failed: ")
                                 + std::strerror(errno));
    }

    std::size_t readSize = static_cast<std::size_t>(bytesRead);
    if (_receiveBuffer.size() > MAX_BOT_RECEIVE_BUFFER
        || readSize > MAX_BOT_RECEIVE_BUFFER - _receiveBuffer.size())
        throw std::runtime_error("bot receive buffer limit exceeded");

    _receiveBuffer.append(chunk, readSize);

    std::string line;
    while (extractLine(line))
        processLine(line);
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

    std::size_t wireSize = newlinePosition + 1;
    if (wireSize > MAX_IRC_MESSAGE_SIZE)
        throw std::runtime_error("bot received an oversized IRC line");

    line = _receiveBuffer.substr(0, newlinePosition);
    _receiveBuffer.erase(0, wireSize);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\0' || line[i] == '\r')
            throw std::runtime_error("bot received a forbidden control byte");
    }
    return true;
}

//oadouz
void Bot::processLine(const std::string &line)
{
    std::cout << "[bot] < " << line << std::endl;

    if (line.compare(0, 5, "PING ") == 0)
    {
        std::string token = line.substr(5);
        queueLine("PONG " + token);
        return;
    }

    if (!_joinSent && line.find(" 001 ") != std::string::npos)
    {
        queueLine("JOIN " + _channel);
        _joinSent = true;
        return;
    }

    if (line.find(" 433 ") != std::string::npos)
        throw std::runtime_error("bot nickname is already in use");

    if (line.find(" 464 ") != std::string::npos)
        throw std::runtime_error("bot password was rejected by the IRC server");

    if (line.find(" 471 ") != std::string::npos
        || line.find(" 473 ") != std::string::npos
        || line.find(" 475 ") != std::string::npos
        || line.find(" 476 ") != std::string::npos)
        throw std::runtime_error("bot could not join the requested channel");

    if (line.compare(0, 6, "ERROR ") == 0)
        throw std::runtime_error("IRC server returned an ERROR message");

    processPrivmsg(line);
}

//oadouz
void Bot::processPrivmsg(const std::string &line)
{
    if (line.empty() || line[0] != ':')
        return;

    std::size_t commandPosition = line.find(" PRIVMSG ");
    if (commandPosition == std::string::npos)
        return;

    std::string prefix = line.substr(1, commandPosition - 1);
    std::size_t nicknameEnd = prefix.find('!');
    std::string sender = prefix.substr(0, nicknameEnd);
    if (sender.empty()
        || toIrcLowerCase(sender) == toIrcLowerCase(_nickname))
        return;

    std::size_t targetStart = commandPosition + 9;
    std::size_t targetEnd = line.find(' ', targetStart);
    if (targetEnd == std::string::npos)
        return;

    std::string target = line.substr(targetStart, targetEnd - targetStart);
    std::size_t messageStart = line.find(" :", targetEnd);
    if (messageStart == std::string::npos)
        return;

    std::string message = line.substr(messageStart + 2);
    std::string replyTarget;

    if (toIrcLowerCase(target) == toIrcLowerCase(_channel))
        replyTarget = _channel;
    else if (toIrcLowerCase(target) == toIrcLowerCase(_nickname))
        replyTarget = sender;
    else
        return;

    if (message == "!help")
        sendPrivmsg(replyTarget, "Commands: !help, !ping, !about, !echo <text>");
    else if (message == "!ping")
        sendPrivmsg(replyTarget, "PONG " + sender);
    else if (message == "!about")
        sendPrivmsg(replyTarget, "I am the ft_irc C++98 bonus bot.");
    else if (message.compare(0, 6, "!echo ") == 0)
        sendPrivmsg(replyTarget, message.substr(6));
    else if (!message.empty() && message[0] == '!')
        sendPrivmsg(replyTarget, "Unknown command. Try !help");
}

//oadouz
void Bot::sendPrivmsg(const std::string &target, const std::string &message)
{
    if (target.empty())
        return;

    std::string safeMessage = message;
    if (safeMessage.size() > MAX_BOT_REPLY_SIZE)
        safeMessage.erase(MAX_BOT_REPLY_SIZE);

    queueLine("PRIVMSG " + target + " :" + safeMessage);
}

//oadouz
void Bot::run()
{
    setupBotSignals();
    connectToServer();
    queueRegistration();

    std::cout << "[bot] Connected to " << _host << ":" << _port
              << " as " << _nickname << "." << std::endl;

    while (!g_botShouldStop)
    {
        struct pollfd botPollFd;
        botPollFd.fd = _socketFd;
        botPollFd.events = POLLIN;
        botPollFd.revents = 0;

        if (!_sendBuffer.empty())
            botPollFd.events |= POLLOUT;

        int readyCount = poll(&botPollFd, 1, BOT_POLL_TIMEOUT_MS);
        if (readyCount < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error(std::string("bot poll() failed: ")
                                     + std::strerror(errno));
        }
        if (readyCount == 0)
            continue;

        if (botPollFd.revents & POLLIN)
            receiveInput();
        if (!g_botShouldStop && (botPollFd.revents & POLLOUT))
            flushOutput();
        if (!(botPollFd.revents & POLLIN)
            && (botPollFd.revents & (POLLERR | POLLHUP | POLLNVAL)))
            throw std::runtime_error("bot socket reported an unrecoverable poll event");
    }

    std::cout << "[bot] Stopped." << std::endl;
}
