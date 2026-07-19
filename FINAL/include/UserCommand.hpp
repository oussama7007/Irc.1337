#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class UserCommand : public Command
{
    public:
        UserCommand();
        ~UserCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};