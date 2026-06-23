#include "include/Channel.hpp"
#include <algorithm> 

Channel::Channel(const std::string &name) 
    : _name(name), _topic(""), _isInviteOnly(false), 
      _topicRestricted(false), _password(""), _userLimit(0)
{
}

Channel::~Channel() {}

std::string Channel::getName() const { return _name; }
std::string Channel::getTopic() const { return _topic; }
std::string Channel::getPassword() const { return _password; }
size_t      Channel::getUserLimit() const { return _userLimit; }

bool Channel::isMember(Client *client) const
{
    return std::find(_members.begin(), _members.end(), client) != _members.end();
}

bool Channel::isOperator(Client *client) const
{
    return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

void Channel::addMember(Client *client)
{
    if (!isMember(client))
    {
        _members.push_back(client);
    }
}

void Channel::removeMember(Client *client)
{
    std::vector<Client*>::iterator it = std::find(_members.begin(), _members.end(), client);
    if (it != _members.end())
    {
        _members.erase(it);
    }
    if (isOperator(client))
    {
        removeOperator(client);
    }
}

void Channel::addOperator(Client *client)
{
    if (!isOperator(client))
    {
        _operators.push_back(client);
    }
}

void Channel::removeOperator(Client *client)
{
    std::vector<Client*>::iterator it = std::find(_operators.begin(), _operators.end(), client);
    if (it != _operators.end())
    {
        _operators.erase(it);
    }
}

void Channel::broadcastMessage(const std::string &message, Client *sender)
{
 
    for (size_t i = 0; i < _members.size(); ++i)
    {

        if (_members[i] == sender) // bach khouna li sift message man3awedch nsift lih message nskipih
            continue;
        _members[i]->sendMessage(message);
    }
}