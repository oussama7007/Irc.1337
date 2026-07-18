#pragma once

#include <string>

//oadouz
class Bot
{
    private:
        std::string _host;
        int         _port;
        std::string _password;
        std::string _channel;

        int         _socketFd;
        std::string _receiveBuffer;

        Bot(const Bot &other);
        Bot &operator=(const Bot &other);

        void connectToServer();
        void sendLine(const std::string &line);
        void receiveMessages();
        bool extractLine(std::string &line);
        void processLine(const std::string &line);
        void greetJoinedUser(const std::string &line);

    public:
        Bot(const std::string &host, int port, const std::string &password, const std::string &channel);
        ~Bot();

        void run();
};
