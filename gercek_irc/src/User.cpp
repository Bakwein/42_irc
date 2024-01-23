#include "User.hpp"

#include "Channel.hpp"
#include "Server.hpp"

User::User(int fd) : fd(fd), isAuth(false) {}
User::~User() {}

int User::getFd() const { return this->fd; }
bool User::getIsAuth() const { return this->isAuth; }
bool User::getIsOperator() const { return this->isOperator; }
std::string User::getNickName() const { return this->nickName; }
std::string User::getUserName() const { return this->userName; }
std::string User::getRealName() const { return this->realName; }

void User::setIsAuth(bool isAuth) { this->isAuth = isAuth; }

void User::sendMsg(std::string msg) const {
    fcntl(this->fd, F_SETFL, O_NONBLOCK);
    if (send(this->fd, msg.c_str(), msg.length(), 0) < 0)
        std::cout << RED << "ERROR: impossible to send" << RESET << std::endl;
    else
        std::cout << YELLOW << "[fd: " << this->getFd() << " (" << this->getNickName() << ")] " << RESET << msg;
}

void User::closeConnection() {
    this->quitAllChannels();
    std::cout << MAGENTA << "Connection closed with client fd: " << this->getFd() << RESET << std::endl;
    close(this->getFd());
    std::deque<int>::iterator index = std::find(Server::fds.begin(), Server::fds.end(), this->getFd());
    Server::fds.erase(index);
    std::deque<User>::iterator user = std::find(Server::users.begin(), Server::users.end(), *this);
    Server::users.erase(user);
}

void User::quitAllChannels(void) {
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        for (std::deque<User *>::iterator user = it->users.begin(); user != it->users.end(); user++) {
            if ((*user)->getFd() != this->getFd()) continue;
            it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " PART " + it->getName() + "\r\n");
            it->users.erase(user);
            break;
        }
    }
}

void User::kickChannel(std::deque<std::string> const &cmd, std::string const &rawcmd) {
    if (cmd.size() < 3 || cmd.at(2).empty()) //burayi 3 yaptim
    {
        this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
        return;
    }
    if (cmd.at(1).empty() || cmd[1][0] != '#') {
        this->sendMsg(":" + Server::name + " 403 " + this->nickName + " " + lower(cmd.at(1)) + " :No such channel\r\n");
        if (cmd.at(3).empty() || cmd[3][0] != ':') {
            this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
            return;
        }
        return;
    }
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == lower(cmd.at(1))) {
            std::deque<std::string>::iterator op = std::find(it->operatorUsers.begin(), it->operatorUsers.end(), this->getNickName());
            if (op == it->operatorUsers.end()) {
                this->sendMsg("482 " + this->getNickName() + " :You're not channel operator\r\n");
                return;
            }
            for (std::deque<User *>::iterator user = it->users.begin(); user != it->users.end(); user++) {
                std::cout << cmd.at(2) << " " << (*user)->getNickName() << std::endl;
                if ((*user)->getNickName() == cmd.at(2)) {
                    if(cmd.size() == 3)
                    {
                        it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost KICK " + cmd.at(1) + " " + cmd.at(2) + " " + "\r\n");
                    }
                    else
                    {
                       it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost KICK " + cmd.at(1) + " " + cmd.at(2) + " " + rawcmd.substr(rawcmd.find(":")) + "\r\n");         
                    }
                    it->users.erase(user);
                    return;
                }
                if (user == it->users.end()) {
                    this->sendMsg("442 " + cmd.at(2) + " :Not on channel\r\n");
                    return;
                }
            }
        }
    }
}

void User::leaveChannel(std::deque<std::string> const &cmd) {
    std::cout << "cmd size ->" << cmd.size() << std::endl;
    if (cmd.size() < 2) //burayi degistirdim!!!
    {
        this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
        return;
    }
    if (cmd[1][0] != '#') {
        this->sendMsg(":" + Server::name + " 403 " + this->nickName + " " + lower(cmd.at(1)) + " :No such channel\r\n");
        return;
    }
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == lower(cmd.at(1))) {
            std::deque<User *>::iterator user = std::find(it->users.begin(), it->users.end(), this);
            if (user == it->users.end()) {
                this->sendMsg("442 " + it->getName() + " :You're not on that channel\r\n");
                return;
            }
            it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " PART " + it->getName() + "\r\n");
            it->users.erase(user);
            return;
        }
    }
}

void User::joinChannel(std::string name, bool checkInviteOnly, std::string password) {
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == name) {
            // Check invite-only
            if (it->isInviteOnly && checkInviteOnly && !it->isOperator(this->getNickName())) {
                this->sendMsg("473 " + this->getNickName() + " " + it->getName() + " :Cannot join channel (+i)\r\n");
                return;
            }
            // Check chan limit
            if (it->users.size() >= it->userLimit && !it->isOperator(this->getNickName())) {
                this->sendMsg("471 " + this->getNickName() + " " + it->getName() + " :Cannot join Channel\r\n");
                return;
            }

            // Check password
            if (it->password != password && !it->isOperator(this->getNickName()) && password != it->password) {
                this->sendMsg("475 " + this->getNickName() + " " + it->getName() + " :Cannot join channel (+k)\r\n");
                return;
            }

            it->users.push_back(this);

            // JOIN message
            std::cout << ":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " JOIN " + name + "\r\n";
            it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " JOIN " + name + "\r\n");

            // RPL_TOPIC
            if (it->topic != "")
                it->sendMsg("332 " + this->getNickName() + " " + name + " :" + it->topic + "\r\n");

            // RPL_NAMREPLY
            std::string userList;
            std::deque<User *>::iterator it2;
            for (it2 = it->users.begin(); it2 != it->users.end(); it2++) {
                if (it2 != it->users.begin())
                    userList += " ";
                if (it->isOperator((*it2)->getNickName())) {
                    userList += "@";
                }
                userList += (*it2)->getNickName();
            }
            this->sendMsg(":" + Server::name + " 353 " + this->getNickName() + " = " + name + " :" + userList + "\r\n");
            this->sendMsg(":" + Server::name + " 366 " + this->getNickName() + " " + name + " :End of /NAMES list.\r\n");

            return;
        }
    }
    Server::addChannel(name, this->getNickName());

    this->joinChannel(name, checkInviteOnly, password);
}

void User::topic(std::deque<std::string> cmd, std::string rawcmd) {
    if (cmd.size() < 2 || cmd[1].empty() || cmd[2].empty() || cmd[2][0] != ':') {
        this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
        return;
    }
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == cmd.at(1) && (it->isTopicFree || it->isOperator(this->getNickName()))) {
            it->changeTopic(rawcmd.substr(rawcmd.find(":") + 1));
            it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " TOPIC " + it->getName() + " :" + it->topic + "\r\n");
            return;
        }
    }
}

void User::invite(std::deque<std::string> cmd, std::string rawcmd) {
    (void)rawcmd;
    if (cmd.size() < 3 || cmd.at(1).empty() || cmd.at(2).empty()) {
        this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
        return;
    }
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == lower(cmd.at(2))) break;
    }

    if (it == Server::channels.end()) {
        this->sendMsg("403 " + this->nickName + " " + lower(cmd.at(2)) + " :No such channel\r\n");
        return;
    }

    if (!it->isOperator(this->getNickName())) {
        this->sendMsg("482 " + this->getNickName() + " " + it->getName() + " :You're not channel operator\r\n");
        return;
    }

    std::deque<User *>::iterator user;
    for (user = it->users.begin(); user != it->users.end(); user++) {
        if ((*user)->getNickName() == this->getNickName()) break;
    }
    if (user == it->users.end()) {
        this->sendMsg("442 " + this->getNickName() + " " + it->getName() + " :You're not on that channel\r\n");
        return;
    }

    std::deque<User>::iterator target;
    for (target = Server::users.begin(); target != Server::users.end(); target++) {
        if (target->getNickName() == cmd.at(1)) break;
    }

    if (target == Server::users.end()) {
        this->sendMsg("401 " + cmd.at(1) + " :No such nick\r\n");
        return;
    }

    for (user = it->users.begin(); user != it->users.end(); user++) {
        if ((*user)->getNickName() == cmd.at(1)) break;
    }
    if (user != it->users.end()) {
        this->sendMsg("443 " + cmd.at(1) + " :is already on channel\r\n");
        return;
    }

    target->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " INVITE " + cmd.at(1) + " " + it->getName() + "\r\n");
    target->joinChannel(it->getName(), false, "");
}

void User::channelMode(Channel &chan) {
    std::string mode;
    chan.isInviteOnly ? mode += "+i" : mode += "-i";
    chan.isTopicFree ? mode += "+t" : mode += "-t";
    chan.password.empty() ? mode += "-k" : mode += "+k";
    chan.userLimit == UINT_MAX ? mode += "-l" : mode += "+l";
    chan.sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " MODE " + chan.getName() + " " + mode + "\r\n");
    return;
}

bool User::checkIsOperator(Channel &chan) {
    if (chan.isOperator(this->getNickName()) == false) {
        this->sendMsg("482 " + this->getNickName() + " " + chan.getName() + " :You're not channel operator\r\n");
        return false;
    }
    return true;
}

void User::mode(std::deque<std::string> const &cmd, std::string const &rawcmd) {
    (void)rawcmd;
    if (cmd.size() == 2 && cmd.at(1).empty() == false && cmd[1][0] == '#') {
        std::deque<Channel>::iterator it1;
        for (it1 = Server::channels.begin(); it1 != Server::channels.end(); it1++) {
            if (it1 == Server::channels.end()) {
                this->sendMsg(":" + Server::name + " 403 " + this->nickName + " " + lower(cmd.at(1)) + " :No such channel\r\n");
                return;
            }
            if (it1->getName() == cmd.at(1)) {
                std::string mode;
                it1->isInviteOnly ? mode += "+i" : mode += "-i";
                it1->isTopicFree ? mode += "+t" : mode += "-t";
                it1->password.empty() ? mode += "-k" : mode += "+k";
                it1->userLimit == UINT_MAX ? mode += "-l" : mode += "+l";
                this->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " MODE " + it1->getName() + " " + mode + "\r\n");
                return;
            }
        }
    }
    if (cmd.size() < 3 || cmd.at(1).empty() || cmd.at(2).empty()) {
        this->sendMsg(":" + this->getNickName() + " 461 :Not Enough Parameters\r\n");
        return;
    }
    if (cmd[1][0] != '#' && cmd.size() == 3 && cmd.at(2) == "+i")
        return;
    if (cmd[1][0] != '#') {
        this->sendMsg(":" + Server::name + " 403 " + this->nickName + " " + lower(cmd.at(1)) + " :No such channel\r\n");
        return;
    }
    std::deque<Channel>::iterator it;
    for (it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it == Server::channels.end()) {
            this->sendMsg(":" + Server::name + " 403 " + this->nickName + " " + lower(cmd.at(1)) + " :No such channel\r\n");
            return;
        }
        if (it->getName() == cmd.at(1))
            break;
    }
    if (cmd[2] == "+i" && cmd.size() == 3) {
        if (!this->checkIsOperator(*it)) return;
        it->isInviteOnly = true;
        channelMode(*it);
    } else if (cmd[2] == "-i" && cmd.size() == 3) {
        if (!this->checkIsOperator(*it)) return;
        it->isInviteOnly = false;
        channelMode(*it);
    } else if (cmd[2] == "+t" && cmd.size() == 3) {
        if (!this->checkIsOperator(*it)) return;
        it->isTopicFree = true;
        channelMode(*it);
    } else if (cmd[2] == "-t" && cmd.size() == 3) {
        if (!this->checkIsOperator(*it)) return;
        it->isTopicFree = false;
        channelMode(*it);
    } else if (cmd[2] == "+k" && cmd.size() == 4 && cmd.at(3).empty() == false) {
        if (!this->checkIsOperator(*it)) return;
        it->password = cmd.at(3);
        channelMode(*it);
    } else if (cmd[2] == "-k" && cmd.size() <= 4) {
        if (!this->checkIsOperator(*it)) return;
        it->password.clear();
        channelMode(*it);
    } else if (cmd[2] == "+l" && cmd.size() == 4 && cmd.at(3).empty() == false) {
        if (!this->checkIsOperator(*it)) return;
        if (cmd.at(3).find_first_not_of("0123456789") != std::string::npos) {
            it->sendMsg("501 " + this->getNickName() + " :Unknown MODE flag\r\n");
            return;
        }
        it->userLimit = stoi(cmd.at(3));
        channelMode(*it);
    } else if (cmd[2] == "-l" && cmd.size() == 3) {
        if (!this->checkIsOperator(*it)) return;
        it->userLimit = UINT_MAX;
        channelMode(*it);
    } else if (cmd[2] == "+o" && cmd.size() == 4 && cmd.at(3).empty() == false) {
        if (!this->checkIsOperator(*it)) return;
        std::deque<User *>::iterator us;
        for (us = it->users.begin(); us != it->users.end(); us++) {
            if ((*us)->getNickName() == cmd.at(3)) {
                if (!it->isOperator((*us)->getNickName())) {
                    it->operatorUsers.push_back((*us)->getNickName());
                }
                it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " MODE " + it->getName() + " +o " + (*us)->getNickName() + "\r\n");
                return;
            }
        }
        this->sendMsg("441 " + this->getNickName() + " " + cmd.at(3) + " " + it->getName() + " :They aren't on that channel\r\n");
    } else if (cmd[2] == "-o" && cmd.size() == 4 && cmd.at(3).empty() == false) {
        if (!this->checkIsOperator(*it)) return;
        if (cmd.at(3) == this->getNickName()) {
            it->sendMsg("ERROR: User can't remove himself/herself from operator list\r\n");
            return;
        }
        std::deque<User *>::iterator us;
        for (us = it->users.begin(); us != it->users.end(); us++) {
            if ((*us)->getNickName() == cmd.at(3)) {
                std::string target = (*us)->getNickName();
                if (it->isOperator(target)) {
                    it->operatorUsers.erase(std::remove(it->operatorUsers.begin(), it->operatorUsers.end(), target), it->operatorUsers.end());
                }
                it->sendMsg(":" + this->getNickName() + "!~" + this->getNickName() + "@localhost" + " MODE " + it->getName() + " -o " + cmd.at(3) + "\r\n");
                return;
            }
        }
        this->sendMsg("441 " + this->getNickName() + " " + cmd.at(3) + " " + it->getName() + " :They aren't on that channel\r\n");
    }

    else
        this->sendMsg("472 " + this->nickName + " " + cmd.at(2) + " :is unknown mode char to me\r\n");
}
