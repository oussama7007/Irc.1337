#include "../include/Channel.hpp"
#include <algorithm> 

//si-hamou
Channel::Channel(const std::string &name) 
    : _name(name), _topic(""), _isInviteOnly(false), 
      _topicRestricted(false), _password(""), _userLimit(0)
{
}

//si-hamou
Channel::~Channel() {}

//si-hamou
std::string Channel::getName() const { return _name; }
//si-hamou
std::string Channel::getTopic() const { return _topic; }
//si-hamou
std::string Channel::getPassword() const { return _password; }
//si-hamou
size_t      Channel::getUserLimit() const { return _userLimit; }

//si-hamou
bool Channel::isMember(Client *client) const
{
    return std::find(_members.begin(), _members.end(), client) != _members.end();
}

//si-hamou
bool Channel::isOperator(Client *client) const
{
    return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

//si-hamou
void Channel::addMember(Client *client)
{
    if (!isMember(client))
    {
        _members.push_back(client);
    }
}

//si-hamou
// Fix this together with Server channel ownership: after the final member is
// removed, erase and delete the empty channel so its modes and topic disappear.
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

//si-hamou
void Channel::addOperator(Client *client)
{
    if (!isOperator(client))
    {
        _operators.push_back(client);
    }
}

//si-hamou
void Channel::removeOperator(Client *client)
{
    std::vector<Client*>::iterator it = std::find(_operators.begin(), _operators.end(), client);
    if (it != _operators.end())
    {
        _operators.erase(it);
    }
}

//si-hamou
void Channel::broadcastMessage(const std::string &message, Client *sender)
{
 
    for (size_t i = 0; i < _members.size(); ++i)
    {

        if (_members[i] == sender) // bach khouna li sift message man3awedch nsift lih message nskipih
            continue;
        _members[i]->sendMessage(message);
    }

}

//si-hamou
bool Channel::isInvited(Client *client) const
{
    return std::find(_invitedClients.begin(), _invitedClients.end(), client) != _invitedClients.end();
}

//si-hamou
void Channel::addInvited(Client *client)
{
    if (!isInvited(client))
        _invitedClients.push_back(client);
}

//si-hamou
void Channel::removeInvited(Client *client)
{
    std::vector<Client*>::iterator it = std::find(_invitedClients.begin(), _invitedClients.end(), client);
    if (it != _invitedClients.end())
        _invitedClients.erase(it);
}




//si-hamou
size_t Channel::getMembersCount() const {    return _members.size(); }



//si-hamou
void Channel::setTopic(const std::string &topic) { _topic = topic; }
//si-hamou
void Channel::setInviteOnly(bool mode) { _isInviteOnly = mode; }
//si-hamou
void Channel::setTopicRestricted(bool mode) { _topicRestricted = mode; }
//si-hamou
void Channel::setPassword(const std::string &password) { _password = password; }
//si-hamou
void Channel::setUserLimit(size_t limit) { _userLimit = limit; }

//si-hamou
bool Channel::isInviteOnly() const { return _isInviteOnly; }
//si-hamou
bool Channel::isTopicRestricted() const { return _topicRestricted; }
//si-hamou
std::vector<Client*> Channel::getMembers() const {return _members;}
