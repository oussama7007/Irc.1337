#pragma once

#include "Command.hpp"
#include <vector>
#include <string>

class KickCommand : public Command
{
    public:
        KickCommand();
        ~KickCommand();
        
        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};