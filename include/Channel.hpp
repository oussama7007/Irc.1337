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

        bool                    _isInviteOnly;      // mode +i
        bool                    _topicRestricted;   // mode +t
        
        std::string             _password;          // mode +k 
        size_t                  _userLimit;         // mode +l 

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
        
        void        broadcastMessage(const std::string &message, Client *sender = NULL);// khass message itsafet l ga3 members, null bach message maywselch joj merat l nafs chakhes  

        bool isInvited(Client *client) const;
        void addInvited(Client *client);
        void removeInvited(Client *client);
};