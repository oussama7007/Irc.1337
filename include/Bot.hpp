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
        std::string _nickname;

        int         _socketFd;
        bool        _joinSent;
        std::string _receiveBuffer;
        std::string _sendBuffer;

        Bot(const Bot &other);
        Bot &operator=(const Bot &other);

        void connectToServer();
        void queueRegistration();
        void queueLine(const std::string &line);
        void flushOutput();
        void receiveInput();
        bool extractLine(std::string &line);
        void processLine(const std::string &line);
        void processPrivmsg(const std::string &line);
        void sendPrivmsg(const std::string &target, const std::string &message);

    public:
        Bot(const std::string &host, int port, const std::string &password,
            const std::string &channel, const std::string &nickname);
        ~Bot();

        void run();
};
