#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class NickCommand : public Command
{
    public:
        NickCommand();
        ~NickCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};