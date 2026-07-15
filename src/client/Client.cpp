#include "Client.hpp"

#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>

// I cap queued output so one client that stops reading cannot grow the whole
// server process without a bound.
static const std::size_t MAX_SEND_BUFFER = 65536;

// I keep enough aggregate space for several valid IRC messages received in one
// TCP read while still bounding a client that never sends a newline.
static const std::size_t MAX_RECV_BUFFER = 8192;

// IRC messages are limited to 512 bytes on the wire, including the line ending.
static const std::size_t MAX_IRC_MESSAGE_SIZE = 512;

//oadouz
// I validate the final wire frame in one place so no command can accidentally
// send an oversized line or inject a second IRC message through control bytes.
static bool isValidOutboundMessage(const std::string &message)
{
    if (message.empty() || message.size() > MAX_IRC_MESSAGE_SIZE)
        return false;

    if (message.size() < 2
        || message[message.size() - 2] != '\r'
        || message[message.size() - 1] != '\n')
        return false;

    for (std::size_t i = 0; i + 2 < message.size(); ++i)
    {
        if (message[i] == '\0' || message[i] == '\r' || message[i] == '\n')
            return false;
    }

    return true;
}

//oadouz
Client::Client(int fd, const std::string &ip)
    : _fd(fd),
      _ip(ip),
      _recvBuffer(),
      _sendBuffer(),
      _nickname(),
      _username(),
      _passOk(false),
      _nickSet(false),
      _userSet(false),
      _failedPassAttempts(0),
      _connectedAt(std::time(NULL)),
      _dead(false),
      _closing(false),
      _deadReason()
{
    if (_fd < 0)
        throw std::invalid_argument("Client requires a valid socket descriptor");

    if (_ip.empty())
        throw std::invalid_argument("Client requires a non-empty IP address");

    if (_connectedAt == static_cast<std::time_t>(-1))
        throw std::runtime_error("time() failed while creating Client");
}

//oadouz
Client::~Client()
{

    if (_fd >= 0)
    {
        if (close(_fd) < 0)
        {
            std::cerr << "Warning: close() failed for client fd=" << _fd << ": "
                      << std::strerror(errno) << "." << std::endl;
        }
        _fd = -1;
    }
}

//oadouz
int Client::getFd() const
{
    return _fd;
}

//oadouz
const std::string &Client::getIp() const
{
    return _ip;
}

//oadouz
void Client::sendMessage(const std::string &message)
{
    if (_dead || _closing)
        return;

    if (!isValidOutboundMessage(message))
    {
        std::cerr << "Internal error: refusing an invalid outbound IRC message for fd="
                  << _fd << "." << std::endl;
        return;
    }

    if (_sendBuffer.size() > MAX_SEND_BUFFER
        || message.size() > MAX_SEND_BUFFER - _sendBuffer.size())
    {

        std::cerr << "Error: client fd=" << _fd
                  << " exceeded the send buffer limit. Disconnecting client." << std::endl;
        markDead("send buffer overflow (slow or stuck client)");
        return;
    }

    _sendBuffer += message;
}

//oadouz
void Client::setPassOk(bool value)
{
    _passOk = value;
}

//oadouz
bool Client::hasPassOk() const
{
    return _passOk;
}

//oadouz
const std::string &Client::getNickname() const
{
    return _nickname;
}

//oadouz
void Client::setNickname(const std::string &nickname)
{
    _nickname = nickname;
}

//oadouz
void Client::setNickSet(bool value)
{
    _nickSet = value;
}

//oadouz
bool Client::hasNickSet() const
{
    return _nickSet;
}

//oadouz
const std::string &Client::getUsername() const
{
    return _username;
}

//oadouz
void Client::setUsername(const std::string &username)
{
    _username = username;
}

//oadouz
void Client::setUserSet(bool value)
{
    _userSet = value;
}

//oadouz
bool Client::hasUserSet() const
{
    return _userSet;
}

//oadouz
bool Client::isRegistered() const
{
    return _passOk && _nickSet && _userSet;
}

//oadouz
void Client::recordFailedPassAttempt()
{
    if (_failedPassAttempts < UINT_MAX)
        ++_failedPassAttempts;
}

//oadouz
unsigned int Client::getFailedPassAttempts() const
{
    return _failedPassAttempts;
}

//oadouz
std::time_t Client::getConnectedAt() const
{
    return _connectedAt;
}

//oadouz
bool Client::appendToRecvBuffer(const char *data, std::size_t length)
{

    if (data == NULL && length == 0)
        return true;

    if (data == NULL)
        return false;

    if (_recvBuffer.size() > MAX_RECV_BUFFER
        || length > MAX_RECV_BUFFER - _recvBuffer.size())
        return false;

    _recvBuffer.append(data, length);

    // Without LF, 512 buffered bytes can no longer become a legal IRC frame
    // because the required line ending would push it over the wire limit.
    if (_recvBuffer.find('\n') == std::string::npos
        && _recvBuffer.size() >= MAX_IRC_MESSAGE_SIZE)
        return false;

    return true;
}

//oadouz
bool Client::extractLine(std::string &line)
{
    line.clear();
    std::size_t newlinePos = _recvBuffer.find('\n');
    if (newlinePos == std::string::npos)
        return false;

    std::size_t wireSize = newlinePos + 1;
    if (wireSize > MAX_IRC_MESSAGE_SIZE)
    {
        _recvBuffer.erase(0, wireSize);
        std::cerr << "Error: client fd=" << _fd
                  << " sent an IRC message longer than 512 bytes." << std::endl;
        markDead("IRC message exceeds 512-byte limit");
        return false;
    }

    line = _recvBuffer.substr(0, newlinePos);
    _recvBuffer.erase(0, wireSize);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\0' || line[i] == '\r')
        {
            std::cerr << "Error: client fd=" << _fd
                      << " sent a forbidden control byte in an IRC message." << std::endl;
            markDead("forbidden control byte in IRC message");
            line.clear();
            return false;
        }
    }

    return true;
}

//oadouz
bool Client::hasPendingOutput() const
{
    return !_sendBuffer.empty();
}

//oadouz
const std::string &Client::getSendBuffer() const
{
    return _sendBuffer;
}

//oadouz
void Client::consumeSendBuffer(std::size_t bytesSent)
{
    if (bytesSent > _sendBuffer.size())
    {
        std::cerr << "Internal error: bytes sent exceeded the queued buffer for fd="
                  << _fd << "." << std::endl;
        markDead("invalid send-buffer accounting");
        return;
    }

    if (bytesSent == _sendBuffer.size())
        _sendBuffer.clear();
    else
        _sendBuffer.erase(0, bytesSent);
}

//oadouz
void Client::markDead(const std::string &reason)
{

    if (_dead)
        return;
    _dead = true;

    if (_deadReason.empty())
        _deadReason = reason;
}

//oadouz
bool Client::isDead() const
{
    return _dead;
}

//oadouz
void Client::markClosing(const std::string &reason)
{

    if (_dead || _closing)
        return;
    _closing = true;
    _deadReason = reason;
}

//oadouz
bool Client::isClosing() const
{
    return _closing;
}

//oadouz
const std::string &Client::getDeadReason() const
{
    return _deadReason;
}
