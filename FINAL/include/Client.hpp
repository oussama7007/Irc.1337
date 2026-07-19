#pragma once

#include <string>
#include <cstddef>

class Client
{
    private:
        int         _fd;
        std::string _ip;

        std::string _recvBuffer;
        std::string _sendBuffer;

        std::string _nickname;
        std::string _username;

        bool        _passOk;
        bool        _nickSet;
        bool        _userSet;
        unsigned int _failedPassAttempts;

        bool        _dead;

        bool        _closing;

        std::string _deadReason;

        Client(const Client &other);
        Client &operator=(const Client &other);

    public:
        Client(int fd, const std::string &ip);
        ~Client();

        int                 getFd() const;
        const std::string   &getIp() const;

        void                sendMessage(const std::string &message);
        void                setPassOk(bool value);
        bool                hasPassOk() const;
        const std::string   &getNickname() const;
        void                setNickname(const std::string &nickname);
        void                setNickSet(bool value);
        bool                hasNickSet() const;
        const std::string   &getUsername() const;
        void                setUsername(const std::string &username);
        void                setUserSet(bool value);
        bool                hasUserSet() const;
        bool                isRegistered() const;
        void                recordFailedPassAttempt();
        unsigned int        getFailedPassAttempts() const;

        bool                appendToRecvBuffer(const char *data, std::size_t length);
        bool                extractLine(std::string &line);
        bool                hasPendingOutput() const;
        const std::string   &getSendBuffer() const;
        void                consumeSendBuffer(std::size_t bytesSent);

        void                markDead(const std::string &reason);
        bool                isDead() const;
        void                markClosing(const std::string &reason);
        bool                isClosing() const;
        const std::string   &getDeadReason() const;
};
