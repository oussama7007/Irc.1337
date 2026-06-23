#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class PartCommand : public Command
{
    public:
        PartCommand();
        ~PartCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};