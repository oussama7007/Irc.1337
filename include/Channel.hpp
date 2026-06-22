

#paragma once 

#include <string>
#include <vector>
#include "Client.hpp"

class Channel 
{
    private:
        std::string name;
        std::string topic;

        std::vector<Client*> members;
        std::vector<Client*> operators; // admins 

        bool    isInviteOnly; // mode +i 
        bool    topicRestricted  // mode +t 
        size_t                  userLimit; // mode +k
        std::string             password; // mode +l
        bool 
}
