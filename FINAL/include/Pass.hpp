#pragma once

#include "Command.hpp"

//oadouz
class PassCommand : public Command
{
    public:
        PassCommand();
        ~PassCommand();

        void execute(Server &server, Client &client, const std::vector<std::string> &params);
};
