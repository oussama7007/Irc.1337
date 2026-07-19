#pragma once

#include <string>
#include <vector>
#include "Client.hpp" 

class Channel
{
    private:
        std::string             _name;
        std::string             _topic;
        
        std::vector<Client*>    _members;
        std::vector<Client*>    _operators; 

        bool                    _isInviteOnly;      
        bool                    _topicRestricted;   
        
        std::string             _password;         
        size_t                  _userLimit;     

        std::vector<Client*>    _invitedClients;

    public:
    
        Channel(const std::string &name);
        ~Channel();

        std::string getName() const;
        std::string getTopic() const;
        std::string getPassword() const;
        size_t      getUserLimit() const;
        
    
        void        addMember(Client *client);
        void        removeMember(Client *client);
        void        addOperator(Client *client);
        void        removeOperator(Client *client);
        
        bool        isMember(Client *client) const;
        bool        isOperator(Client *client) const;
        
        void        broadcastMessage(const std::string &message, Client *sender = NULL);

        bool isInvited(Client *client) const;
        void addInvited(Client *client);
        void removeInvited(Client *client);

        void setInviteOnly(bool mode);
        void setTopicRestricted(bool mode);
        void setPassword(const std::string &password);
        void setUserLimit(size_t limit);
        
        void    setTopic(const std::string &topic);
        bool isInviteOnly() const;
        bool isTopicRestricted() const;
        size_t getMembersCount() const;
        std::vector<Client*> getMembers() const;
};