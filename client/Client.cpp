// src/client/Client.cpp  [PARTNER'S PART — placeholder]
#include "include/Client.hpp"

Client::Client(int fd) : _fd(fd), _passOk(false), _nickSet(false), _userSet(false) {}

int Client::getFd() const { return _fd; }
std::string Client::getNickname() const { return _nickname; }
std::string Client::getUsername() const { return _username; }
bool Client::isRegistered() const { return _passOk && _nickSet && _userSet; }

void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }
void Client::setPassOk(bool ok) { _passOk = ok; }
void Client::setNickSet(bool ok) { _nickSet = ok; }
void Client::setUserSet(bool ok) { _userSet = ok; }

void Client::appendToInBuffer(const std::string &data) { _inBuffer += data; }

bool Client::hasCompleteLine() const
{
    return _inBuffer.find('\n') != std::string::npos;
}

std::string Client::popLine()
{
    size_t pos = _inBuffer.find('\n');
    std::string line = _inBuffer.substr(0, pos); // trailing \r gets stripped in Parser
    _inBuffer.erase(0, pos + 1);
    return line;
}

void Client::sendMessage(const std::string &msg) { _outBuffer += msg; }
bool Client::hasDataToSend() const { return !_outBuffer.empty(); }
std::string& Client::getOutBuffer() { return _outBuffer; }
void Client::clearOutBuffer(size_t n) { _outBuffer.erase(0, n); }