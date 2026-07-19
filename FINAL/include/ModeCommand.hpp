#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class ModeCommand : public Command
{
    public:
        ModeCommand();
        ~ModeCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};