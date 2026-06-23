// include/Client.hpp  [PARTNER'S PART — placeholder]
#pragma once
#include <string>

class Client
{
    private:
        int         _fd;
        std::string _nickname;
        std::string _username;
        std::string _inBuffer;   // raw bytes received, not yet split into lines
        std::string _outBuffer;  // bytes queued to send, not yet flushed to the socket
        bool        _passOk;
        bool        _nickSet;
        bool        _userSet;

    public:
        Client(int fd);

        int          getFd() const;
        std::string  getNickname() const;
        std::string  getUsername() const;
        bool         isRegistered() const;  // true once PASS + NICK + USER all succeeded

        void         setNickname(const std::string &nick);
        void         setUsername(const std::string &user);
        void         setPassOk(bool ok);
        void         setNickSet(bool ok);
        void         setUserSet(bool ok);

        void         appendToInBuffer(const std::string &data);
        bool         hasCompleteLine() const;
        std::string  popLine();             // extracts + removes one line up to '\n'

        void         sendMessage(const std::string &msg); // queues into outBuffer, doesn't send directly
        bool         hasDataToSend() const;
        std::string& getOutBuffer();
        void         clearOutBuffer(size_t n);
};