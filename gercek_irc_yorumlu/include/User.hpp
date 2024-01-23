#pragma once

#include "irc.hpp"

class Channel;

class User {
   public:
    User(int fd);
    ~User();

    int getFd() const;
    bool getIsAuth() const;
    bool getIsOperator() const;
    std::string getNickName() const;
    std::string getUserName() const;
    std::string getRealName() const;

    void setIsAuth(bool isAuth);

    void channelMode(Channel & chan);
    void sendMsg(std::string msg) const;
    void joinChannel(std::string name, bool checkInviteOnly, std::string password);
    void leaveChannel(std::deque<std::string> const& cmd);
    void kickChannel(std::deque<std::string> const& cmd, std::string const& rawcmd);
    void closeConnection();
    void topic(std::deque<std::string> cmd, std::string rawcmd);
    void invite(std::deque<std::string> cmd, std::string rawcmd);
    void quitAllChannels(void);
    void mode(std::deque<std::string> const& cmd, std::string const& rawcmd);
    bool checkIsOperator(Channel &chan);

    std::string input;

    int fd;
    bool isAuth;
    bool isOperator;
    std::string nickName;
    std::string userName;
    std::string realName;

    bool operator==(const User& other) const {
        return (this->fd == other.fd);
    }
};
