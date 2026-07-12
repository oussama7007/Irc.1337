#include "Client.hpp"

#include <unistd.h>
#include <iostream>

static const std::size_t MAX_SEND_BUFFER = 65536;

static const std::size_t MAX_RECV_BUFFER = 8192;

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
      _dead(false),
      _closing(false),
      _deadReason()
{
}

//oadouz
Client::~Client()
{

    if (_fd >= 0)
    {
        close(_fd);
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

    if (_sendBuffer.size() + message.size() > MAX_SEND_BUFFER)
    {

        std::cerr << "Error: client fd=" << _fd << " exceeded the send buffer limit ("
                  << _sendBuffer.size() + message.size() << " bytes). Disconnecting client." << std::endl;
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
bool Client::isRegistered() const
{
    return _passOk && _nickSet && _userSet;
}

//oadouz
bool Client::appendToRecvBuffer(const char *data, std::size_t length)
{

    if (data == NULL)
        return true;

    if (_recvBuffer.size() + length > MAX_RECV_BUFFER)
        return false;

    _recvBuffer.append(data, length);
    return true;
}

//oadouz
bool Client::extractLine(std::string &line)
{

    std::size_t newlinePos = _recvBuffer.find('\n');
    if (newlinePos == std::string::npos)
        return false;

    line = _recvBuffer.substr(0, newlinePos);
    _recvBuffer.erase(0, newlinePos + 1);

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

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

    if (bytesSent >= _sendBuffer.size())
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
