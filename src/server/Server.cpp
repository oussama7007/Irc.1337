#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Parser.hpp"
#include "Command.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <stdexcept>

// I keep the pending kernel queue bounded and large enough for a burst of
// legitimate connections while the single poll loop catches up.
static const int LISTEN_BACKLOG = 128;

// One recv() handles a bounded chunk so a busy client cannot monopolize one
// event-loop iteration.
static const std::size_t RECV_CHUNK_SIZE = 4096;

// These limits preserve the tested 128-client local workload while placing a
// hard bound on descriptors controlled by one address and by the whole server.
static const std::size_t MAX_CLIENTS_PER_IP = 128;
static const std::size_t MAX_TOTAL_CLIENTS = 512;

static volatile sig_atomic_t g_shutdown = 0;

//oadouz
// I check close() even though cleanup cannot safely retry it, because a hidden
// descriptor cleanup failure must still be visible during debugging.
static void closeSocket(int fd, const std::string &context)
{
    if (fd < 0)
        return;

    if (close(fd) < 0)
    {
        std::cerr << "Warning: close() failed for " << context << ", fd=" << fd << ": " << std::strerror(errno) << "." << std::endl;
    }
}

//oadouz
static std::string getClientDisplayName(const Client *client)
{
    if (client == NULL || client->getNickname().empty())
        return "*";
    return client->getNickname();
}

//oadouz
static void handleShutdownSignal(int signalNumber)
{
    (void)signalNumber;
    g_shutdown = 1;
}

//si-hamou
// Keep these factories with the dispatcher and command-handler implementation.
Command *createPartCommand();
Command *createPrivmsgCommand();
Command *createKickCommand();
Command *createInviteCommand();
Command *createModeCommand();

//si-hamou
// This normalization is part of command dispatch, not server transport.
static std::string toUpperCase(const std::string &text)
{
    std::string result = text;

    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}

//oadouz
// I use the IRC casemapping here because IRC identity is not compared with
// normal byte-for-byte string rules. Every nickname lookup must agree.
static std::string toIrcLowerCase(const std::string &text)
{
    std::string result = text;

    for (std::size_t i = 0; i < result.size(); ++i)
    {
        unsigned char character = static_cast<unsigned char>(result[i]);

        if (character >= 'A' && character <= 'Z')
            result[i] = static_cast<char>(character - 'A' + 'a');
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
Server::Server(int port, const std::string &password)
    : _port(port),
      _password(password),
      _listenFd(-1)
{
    if (_port < 1 || _port > 65535)
        throw std::invalid_argument("server port must be between 1 and 65535");

    if (_password.empty())
        throw std::invalid_argument("server password must not be empty");
}

//oadouz
Server::~Server()
{

    for (std::map<std::string, Command*>::iterator it = _commands.begin(); it != _commands.end(); ++it)
        delete it->second;
    _commands.clear();

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    _clients.clear();

    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    _channels.clear();

    if (_listenFd >= 0)
    {
        closeSocket(_listenFd, "listening socket");
        _listenFd = -1;
    }
}

//oadouz
void Server::setupSignals()
{
    struct sigaction ignoreAction;
    struct sigaction shutdownAction;

    std::memset(&ignoreAction, 0, sizeof(ignoreAction));
    std::memset(&shutdownAction, 0, sizeof(shutdownAction));

    ignoreAction.sa_handler = SIG_IGN;

    if (sigemptyset(&ignoreAction.sa_mask) < 0)
        throw std::runtime_error(std::string("sigemptyset() failed for SIGPIPE: ") + std::strerror(errno));

    if (sigaction(SIGPIPE, &ignoreAction, NULL) < 0)
        throw std::runtime_error(std::string("sigaction(SIGPIPE) failed: ") + std::strerror(errno));

    shutdownAction.sa_handler = handleShutdownSignal;
    if (sigemptyset(&shutdownAction.sa_mask) < 0)
        throw std::runtime_error(std::string("sigemptyset() failed for shutdown signals: ") + std::strerror(errno));

    shutdownAction.sa_flags = 0;

    if (sigaction(SIGINT, &shutdownAction, NULL) < 0)
        throw std::runtime_error(std::string("sigaction(SIGINT) failed: ") + std::strerror(errno));
    if (sigaction(SIGTERM, &shutdownAction, NULL) < 0)
        throw std::runtime_error(std::string("sigaction(SIGTERM) failed: ") + std::strerror(errno));
    if (sigaction(SIGQUIT, &shutdownAction, NULL) < 0)
        throw std::runtime_error(std::string("sigaction(SIGQUIT) failed: ") + std::strerror(errno));
}

//oadouz
void Server::setupSocket()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw std::runtime_error(std::string("socket() failed: ") + std::strerror(errno));

    int reuseFlag = 1;

    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(reuseFlag)) < 0)
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno));

    // The subject permits fcntl() only with F_SETFL and O_NONBLOCK.
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error(std::string("fcntl(O_NONBLOCK) failed: ") + std::strerror(errno));

    struct sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(static_cast<unsigned short>(_port));

    if (bind(_listenFd, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0)
        throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno) + " (the port may be in use, or ports below 1024 may require elevated privileges)");

    if (listen(_listenFd, LISTEN_BACKLOG) < 0)
        throw std::runtime_error(std::string("listen() failed: ") + std::strerror(errno));

    struct pollfd listenPollFd;
    listenPollFd.fd = _listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);
}

//si-hamou
// This registry belongs to command dispatch. Validate every returned factory
// pointer before inserting it so a missing handler cannot be dereferenced.
void Server::setupCommands()
{
    _commands["PASS"] = createPassCommand();
    _commands["NICK"] = createNickCommand();
    _commands["USER"] = createUserCommand();
    _commands["JOIN"] = createJoinCommand();
    _commands["PART"] = createPartCommand();
    _commands["PRIVMSG"] = createPrivmsgCommand();
    _commands["KICK"] = createKickCommand();
    _commands["INVITE"] = createInviteCommand();
    _commands["TOPIC"] = createTopicCommand();
    _commands["MODE"] = createModeCommand();
}

//oadouz
void Server::run()
{
    setupSignals();
    setupSocket();
    setupCommands();

    std::cout << "[ft_irc] Server is listening on port " << _port << "." << std::endl;

    while (!g_shutdown)
    {
        refreshPollEvents();

        if (_pollFds.empty())
            throw std::runtime_error("poll() cannot run without the listening socket");

        int readyCount = poll(&_pollFds[0], static_cast<nfds_t>(_pollFds.size()), -1);
        if (readyCount < 0)
        {

            if (errno == EINTR)
                continue;

            throw std::runtime_error(std::string("poll() failed: ") + std::strerror(errno));
        }

        for (std::size_t i = 0; i < _pollFds.size(); ++i)
        {
            short returnedEvents = _pollFds[i].revents;
            int currentFd = _pollFds[i].fd;

            if (returnedEvents == 0)
                continue;

            if (currentFd == _listenFd)
            {

                if (returnedEvents & POLLIN)
                    acceptNewClient();
                else
                    throw std::runtime_error("listening socket failed (POLLERR/POLLHUP)");
                continue;
            }

            if (returnedEvents & POLLIN)
                handleClientRead(currentFd);

            if (returnedEvents & POLLOUT)
                handleClientWrite(currentFd);

            if (!(returnedEvents & POLLIN) && (returnedEvents & (POLLERR | POLLHUP | POLLNVAL)))
            {
                Client *erroredClient = getClientByFd(currentFd);
                if (erroredClient != NULL)
                    erroredClient->markDead("socket error (POLLERR/POLLHUP)");
            }
        }

        reapDeadClients();
    }

    std::cout << "[ft_irc] Shutdown signal received. Cleaning up." << std::endl;
}

//oadouz
void Server::refreshPollEvents()
{

    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        _pollFds[i].revents = 0;

        if (_pollFds[i].fd == _listenFd)
        {
            _pollFds[i].events = POLLIN;
            continue;
        }

        Client *client = getClientByFd(_pollFds[i].fd);

        if (client == NULL)
        {
            throw std::runtime_error("poll state contains a client fd with no Client object");
        }

        if (client->isClosing())
        {
            if (client->hasPendingOutput())
                _pollFds[i].events = POLLOUT;
            else
                _pollFds[i].events = 0;
            continue;
        }

        if (client->hasPendingOutput())
            _pollFds[i].events = POLLIN | POLLOUT;
        else
            _pollFds[i].events = POLLIN;
    }
}

//oadouz
void Server::acceptNewClient()
{
    struct sockaddr_in clientAddress;
    socklen_t addressLength = sizeof(clientAddress);

    int clientFd = accept(_listenFd, reinterpret_cast<struct sockaddr*>(&clientAddress), &addressLength);

    if (clientFd < 0)
    {
        int acceptError = errno;

        if (acceptError == EAGAIN || acceptError == EWOULDBLOCK || acceptError == EINTR)
            return;

        std::cerr << "Warning: accept() failed: " << std::strerror(acceptError) << ". Continuing to listen for connections." << std::endl;
        return;
    }

    if (_clients.size() >= MAX_TOTAL_CLIENTS)
    {
        std::cerr << "Warning: rejecting fd=" << clientFd << " because the global client limit was reached." << std::endl;
        closeSocket(clientFd, "rejected client socket");
        return;
    }

    // The subject permits fcntl() only with F_SETFL and O_NONBLOCK.
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Error: fcntl(F_SETFL) failed for fd=" << clientFd << ": " << std::strerror(errno) << ". Closing connection." << std::endl;
        closeSocket(clientFd, "client socket after fcntl failure");
        return;
    }

    char ipBuffer[INET_ADDRSTRLEN];
    const char *ipText = inet_ntop(AF_INET, &clientAddress.sin_addr, ipBuffer, sizeof(ipBuffer));

    if (ipText == NULL)
    {
        std::cerr << "Error: inet_ntop() failed for fd=" << clientFd << ": " << std::strerror(errno) << ". Closing connection." << std::endl;
        closeSocket(clientFd, "client socket after inet_ntop failure");
        return;
    }

    std::string clientIp = ipText;

    if (countClientsFromIp(clientIp) >= MAX_CLIENTS_PER_IP)
    {
        std::cerr << "Warning: rejecting a connection from " << clientIp << " because the per-IP client limit was reached." << std::endl;
        closeSocket(clientFd, "client socket rejected by per-IP limit");
        return;
    }

    Client *newClient = NULL;
    try
    {
        newClient = new Client(clientFd, clientIp);
        _clients[clientFd] = newClient;

        struct pollfd clientPollFd;
        clientPollFd.fd = clientFd;
        clientPollFd.events = POLLIN;
        clientPollFd.revents = 0;
        _pollFds.push_back(clientPollFd);
    }
    catch (const std::exception &e)
    {

        std::cerr << "Error: failed to register new client (" << e.what() << "). Rolling back connection." << std::endl;
        if (newClient != NULL)
        {
            _clients.erase(clientFd);
            delete newClient;
        }
        else
            closeSocket(clientFd, "client socket after allocation failure");
        return;
    }

    std::cout << "[+] Client connected: fd=" << clientFd << ", ip=" << clientIp << "." << std::endl;
}

//oadouz
void Server::handleClientRead(int fd)
{
    Client *client = getClientByFd(fd);

    if (client == NULL)
    {
        std::cerr << "Internal error: POLLIN event for unknown fd=" << fd << "." << std::endl;
        return;
    }

    if (client->isDead() || client->isClosing())
        return;

    char chunkBuffer[RECV_CHUNK_SIZE];
    ssize_t bytesRead = recv(fd, chunkBuffer, sizeof(chunkBuffer), 0);

    if (bytesRead == 0)
    {
        client->markClosing("client closed the connection");
        return;
    }

    if (bytesRead < 0)
    {
        int receiveError = errno;

        if (receiveError == EAGAIN || receiveError == EWOULDBLOCK || receiveError == EINTR)
            return;

        client->markDead(std::string("recv() failed: ") + std::strerror(receiveError));
        return;
    }

    if (!client->appendToRecvBuffer(chunkBuffer, static_cast<std::size_t>(bytesRead)))
    {
        std::cerr << "Error: client fd=" << fd << " exceeded the receive buffer limit without a newline. Disconnecting client." << std::endl;
        client->markDead("input flood: recv buffer overflow");
        return;
    }

    std::string completedLine;

    while (client->extractLine(completedLine))
    {
        processLine(client, completedLine);

        if (client->isDead() || client->isClosing())
            break;
    }
}

//oadouz
void Server::handleClientWrite(int fd)
{
    Client *client = getClientByFd(fd);

    if (client == NULL)
    {
        std::cerr << "Internal error: POLLOUT event for unknown fd=" << fd << "." << std::endl;
        return;
    }
    if (client->isDead())
        return;

    const std::string &outgoingData = client->getSendBuffer();

    if (outgoingData.empty())
        return;

    ssize_t bytesSent = send(fd, outgoingData.c_str(), outgoingData.size(), 0);

    if (bytesSent < 0)
    {
        int sendError = errno;

        if (sendError == EAGAIN || sendError == EWOULDBLOCK || sendError == EINTR)
            return;

        client->markDead(std::string("send() failed: ") + std::strerror(sendError));
        return;
    }

    if (bytesSent == 0)
    {
        client->markDead("send() returned zero bytes");
        return;
    }

    client->consumeSendBuffer(static_cast<std::size_t>(bytesSent));
}

//si-hamou
// Fix CAP negotiation state and keep QUIT reasons unchanged instead of adding
// a second "Quit:" prefix. Replace its ternary expressions with explicit if
// statements to follow the agreed project style. This dispatcher is Si Hamou's.
void Server::processLine(Client *client, const std::string &rawLine)
{

    if (client == NULL)
        return;

    Message message = Parser::parse(rawLine);

    if (message.command.empty())
        return;

    std::string commandName = toUpperCase(message.command);

    if (commandName == "CAP")
    {

        if (!message.params.empty() && toUpperCase(message.params[0]) == "LS")
            client->sendMessage(":server CAP * LS :\r\n");
        return;
    }

    if (commandName == "PING")
    {

        std::string pingToken = message.params.empty() ? std::string("ft_irc") : message.params[0];
        client->sendMessage(":server PONG server :" + pingToken + "\r\n");
        return;
    }

    if (commandName == "QUIT")
    {
        std::string quitReason = message.params.empty() ? std::string("Client Quit") : message.params[0];

        client->markClosing("Quit: " + quitReason);
        return;
    }

    std::map<std::string, Command*>::iterator commandIt = _commands.find(commandName);

    if (commandIt == _commands.end())
    {

        std::string displayName = client->getNickname().empty() ? std::string("*") : client->getNickname();
        client->sendMessage(":server 421 " + displayName + " " + commandName + " :Unknown command\r\n");
        return;
    }

    if (commandName != "PASS" && !client->hasPassOk())
    {
        client->sendMessage(":server 451 * :You have not registered\r\n");
        return;
    }

    if (commandName != "PASS" && commandName != "NICK" && commandName != "USER" && !client->isRegistered())
    {
        std::string displayName = client->getNickname().empty() ? std::string("*") : client->getNickname();
        client->sendMessage(":server 451 " + displayName + " :You have not registered\r\n");
        return;
    }

    try
    {
        commandIt->second->execute(*this, *client, message.params);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: command " << commandName << " threw an exception: " << e.what() << ". Server will continue running." << std::endl;
    }
}

//oadouz
void Server::reapDeadClients()
{
    std::vector<int> deadFds;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {

        if (it->second->isClosing() && !it->second->hasPendingOutput())
            it->second->markDead(it->second->getDeadReason());

        if (it->second->isDead())
            deadFds.push_back(it->first);
    }

    for (std::size_t i = 0; i < deadFds.size(); ++i)
        disconnectClient(deadFds[i]);
}

//oadouz
void Server::disconnectClient(int fd)
{
    std::map<int, Client*>::iterator clientIt = _clients.find(fd);

    if (clientIt == _clients.end())
        return;

    Client *client = clientIt->second;

    std::string displayName = getClientDisplayName(client);
    std::cout << "[-] Client disconnected: fd=" << fd << ", nickname=" << displayName << ", reason=" << client->getDeadReason() << "." << std::endl;

    if (client->isRegistered())
    {
        std::string quitReason = client->getDeadReason();
        if (quitReason.empty())
            quitReason = "Client Quit";

        std::string quitMessage = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :" + quitReason + "\r\n";
        std::vector<Client*> recipients;

        for (std::map<std::string, Channel*>::iterator chIt = _channels.begin(); chIt != _channels.end(); ++chIt)
        {
            Channel *channel = chIt->second;

            if (!channel->isMember(client))
                continue;

            std::vector<Client*> members = channel->getMembers();
            for (std::size_t i = 0; i < members.size(); ++i)
            {
                if (members[i] != client
                    && std::find(recipients.begin(), recipients.end(), members[i])
                       == recipients.end())
                {
                    recipients.push_back(members[i]);
                }
            }
        }

        for (std::size_t i = 0; i < recipients.size(); ++i)
            recipients[i]->sendMessage(quitMessage);
    }

    for (std::map<std::string, Channel*>::iterator chIt = _channels.begin(); chIt != _channels.end(); ++chIt)
    {
        chIt->second->removeMember(client);
        chIt->second->removeInvited(client);
    }

    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd == fd)
        {
            _pollFds.erase(_pollFds.begin() + i);
            break;
        }
    }

    _clients.erase(clientIt);
    delete client;
}

//oadouz
Client *Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);

    if (it == _clients.end())
        return NULL;
    return it->second;
}

//oadouz
std::size_t Server::countClientsFromIp(const std::string &ip) const
{
    if (ip.empty())
        return 0;

    std::size_t count = 0;
    for (std::map<int, Client*>::const_iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second != NULL && !it->second->isDead() && it->second->getIp() == ip)
            ++count;
    }
    return count;
}

//oadouz
const std::string &Server::getPassword() const
{
    return _password;
}

//oadouz
Client *Server::findClientByNick(const std::string &nickname)
{

    if (nickname.empty())
        return NULL;

    const std::string normalizedNickname = toIrcLowerCase(nickname);

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {

        if (!it->second->isDead()
            && toIrcLowerCase(it->second->getNickname()) == normalizedNickname)
            return it->second;
    }
    return NULL;
}

//oadouz
// I send one NICK event per affected client because two users can share more
// than one channel and must not receive the same identity change twice.
void Server::broadcastNicknameChange(Client *client,
                                     const std::string &oldNickname,
                                     const std::string &newNickname)
{
    if (client == NULL || oldNickname.empty() || newNickname.empty())
        return;

    std::string message = ":" + oldNickname + "!" + client->getUsername() + "@localhost NICK :" + newNickname + "\r\n";
    std::vector<Client*> recipients;
    recipients.push_back(client);

    for (std::map<std::string, Channel*>::iterator channelIt = _channels.begin();
         channelIt != _channels.end(); ++channelIt)
    {
        Channel *channel = channelIt->second;

        if (!channel->isMember(client))
            continue;

        std::vector<Client*> members = channel->getMembers();
        for (std::size_t i = 0; i < members.size(); ++i)
        {
            if (std::find(recipients.begin(), recipients.end(), members[i]) == recipients.end())
                recipients.push_back(members[i]);
        }
    }

    for (std::size_t i = 0; i < recipients.size(); ++i)
        recipients[i]->sendMessage(message);
}

//oadouz
Channel *Server::findChannel(const std::string &name)
{
    //si-hamou: Pass one canonical IRC-casemapped channel key here and preserve
    // the first spelling only for display, otherwise shadow channels remain.
    std::map<std::string, Channel*>::iterator it = _channels.find(name);

    if (it == _channels.end())
        return NULL;
    return it->second;
}

//oadouz
Channel *Server::createChannel(const std::string &name)
{
    //si-hamou: Enforce the agreed global and per-client channel quotas before
    // this allocation, and delete the object after its final member leaves.
    Channel *existingChannel = findChannel(name);

    if (existingChannel != NULL)
        return existingChannel;

    Channel *newChannel = new Channel(name);
    _channels[name] = newChannel;
    return newChannel;
}

// ana 
void Server::removeChannel(const std::string& name)
{
    // ملاحظة للصديق ديالك: تأكد بلي السمية ديال الـ map عندك هي _channels 
    // إلى كنتي مسميها channels بلا تيريط، بدلها هنا
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    
    if (it != _channels.end())
    {
        delete it->second;   // 1. كنمسحو القناة من الميموري (تفادي Leaks)
        _channels.erase(it); // 2. كنحيدوها من القائمة ديال السيرفر
    }
}