#pragma once

#include "User.hpp"
#include "irc.hpp"

class Channel {
   public:
    Channel(std::string name, std::string operatorName);
    ~Channel();

    std::string getName() const;

    void sendMsg(std::string msg) const;
    void sendMsgFromUser(std::string msg, User &user) const;
    void changeTopic(std::string cmd);

    bool isOperator(std::string nickName);

    bool isInviteOnly;
    bool isTopicFree;
    const std::string name;
    unsigned int userLimit;
    std::string topic;
    std::deque<User *> users;
    std::deque<std::string> operatorUsers;
    std::string password;
};
