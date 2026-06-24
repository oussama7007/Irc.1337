#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class JoinCommand : public Command
{
    public:
        JoinCommand();
        ~JoinCommand();
        
     
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};