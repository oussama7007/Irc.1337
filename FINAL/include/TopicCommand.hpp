


#pragma once 

#include "Command.hpp"


class TopicCommand : public Command
{
    public:
        TopicCommand();
        ~TopicCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};