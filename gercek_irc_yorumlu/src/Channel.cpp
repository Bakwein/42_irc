#include "Channel.hpp"

Channel::Channel(std::string name, std::string operatorName) : isInviteOnly(false), isTopicFree(true), name(lower(name)), userLimit(UINT_MAX), topic("") {
    this->operatorUsers.push_back(operatorName);
}
Channel::~Channel() {}

std::string Channel::getName() const { return this->name; }

void Channel::sendMsg(std::string msg) const { // tüm userlara o mesaj yollanır
    std::deque<User *>::const_iterator it;
    if (users.size() > 0) {
        for (it = users.begin(); it != users.end(); it++)
            (*it)->sendMsg(msg);
    }
}

void Channel::sendMsgFromUser(std::string msg, User &user) const {
    std::deque<User *>::const_iterator it;
    for (it = users.begin(); it != users.end(); it++) {
        if ((*it)->getFd() == user.getFd())
            continue;
        (*it)->sendMsg(msg);
    }
}

void Channel::changeTopic(std::string cmd) {
    if (cmd.empty())
        this->topic = "";
    else
        this->topic = cmd;
}

bool Channel::isOperator(std::string nickName) {
    return (std::find(this->operatorUsers.begin(), this->operatorUsers.end(), nickName) != this->operatorUsers.end());
}
