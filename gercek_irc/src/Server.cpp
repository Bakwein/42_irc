#include "Server.hpp"

void Server::createSocket(void) {
    int iSetOption = 1;
    try {
        Server::fds.push_back(socket(AF_INET, SOCK_STREAM, 0));
        setsockopt(Server::getServerSocketFd(), SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));
    } catch (std::exception &e) {
        std::cout << RED << "ERRROR: " << e.what() << RESET << std::endl;
        return;
    }
}

void Server::bindSocket(void) {
    bzero((char *)&Server::addr, sizeof(Server::addr));
    Server::addr.sin_family = AF_INET;
    Server::addr.sin_addr.s_addr = INADDR_ANY;
    Server::addr.sin_port = htons(Server::port);

    if (bind(Server::getServerSocketFd(), (struct sockaddr *)&Server::addr, sizeof(Server::addr))) {
        std::cout << RED << "ERROR: bind port: " << strerror(errno) << RESET << std::endl;
        exit(1);
    }
    if (listen(Server::getServerSocketFd(), FD_SETSIZE)) {
        std::cout << RED << "ERROR: bind port: " << strerror(errno) << RESET << std::endl;
        exit(1);
    }
}

void signal_callback_handler(int) {
    Server::exited = 1;
    std::cout << std::endl
              << "=== Good Bye ! ===" << std::endl;
}

void Server::selectSocket(void) {
    int select_output;

    signal(SIGINT, signal_callback_handler);

    while (!Server::exited) {
        FD_ZERO(&Server::read_fd_set);
        for (size_t i = 0; i < Server::fds.size(); i++) {
            FD_SET(Server::fds[i], &Server::read_fd_set);
        } // 1-23456-> 36

        select_output = select(FD_SETSIZE, &Server::read_fd_set, NULL, NULL, NULL);

        if (Server::exited)
            return;

        if (select_output < 0) {
            std::cerr << "Select error" << std::endl;
            return;
        }//1-23456 -> 3
        if (FD_ISSET(Server::getServerSocketFd(), &Server::read_fd_set)) {
            newConnection();
            continue;
        }

        for (size_t i = 0; i < Server::fds.size(); i++) {
            if (!FD_ISSET(Server::fds[i], &Server::read_fd_set))
                continue;

            for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++) {
                if (it->getFd() == Server::fds[i]) {
                    readInput(*it);
                    break;
                }
            }
        }
    }
}

void Server::newConnection(void) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    bzero((char *)&client_addr, addr_len);
    int client_fd = accept(Server::getServerSocketFd(), (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd >= 0) {
        Server::fds.push_back(client_fd);
        Server::users.push_back(User(client_fd));
        std::cout << MAGENTA "New connection with client fd: " << client_fd << RESET << std::endl;
    } else {
        std::cout << RED "ERROR: Failed to connect new client" << RESET << std::endl;
    }
}

void Server::readInput(User &user) {
    int output = 0;
    char buffer[512];

    fcntl(user.getFd(), F_SETFL, O_NONBLOCK);
    bzero(buffer, 512);
    output = recv(user.getFd(), buffer, 512, 0);
    if (output <= 0) {
        user.closeConnection();
        return;
    }
    user.input += buffer; 

    while (user.input.find("\n") != std::string::npos) {
        std::string cmd = user.input.substr(0, user.input.find("\n"));
        user.input = user.input.substr(user.input.find("\n") + 1);
        //std::cout << "ahlk\n";
        if (cmd.size() > 1 && cmd.at(cmd.size() - 1) == '\r')
            cmd = cmd.substr(0, cmd.size() - 1);
        //std::cout << "asdhasdhsadhasdh\n";
        if (executeCommand(user, cmd))
            return;
    }
}

// ###Cmd functions###//

void nick(User &user, std::deque<std::string> &cmd) {
    if (cmd.size() < 2) {
        user.sendMsg(":" + Server::name + " 431 " + user.nickName + " :No nickname given\r\n");
        return;
    }
    if (cmd.at(1).find_first_of("#$:") != std::string::npos || isdigit(cmd[1][0]) == true) {
        user.sendMsg(":" + Server::name + " 432 " + user.nickName + " :Invalid nickname\r\n");
        return;
    }
    for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++) {
        if (it->getNickName() == cmd.at(1) && it->getFd() != user.getFd() && user.getUserName().empty()) {
            cmd.at(1) += std::to_string(rand() % 1000);
            user.nickName = cmd.at(1);
        }
        if (it->getNickName() == cmd.at(1) && it->getFd() != user.getFd()) {
            user.sendMsg(":" + Server::name + " 433 " + user.nickName + ":Nickname is already in use\r\n");
            return;
        }
        
    }
    user.sendMsg(":" + user.nickName + " NICK " + cmd.at(1) + "\r\n");
    user.nickName = cmd.at(1);
}

void sendNoticeMsg(User &user, std::deque<std::string> &cmd, std::string rawcmd) {
    if (cmd.size() == 1) {
        return;
    }
    if (cmd.size() == 2) {
        return;
    }
    if (cmd[2][0] != ':') {
        user.sendMsg("ERROR :Invalid message format\r\n");
        return;
    }
    int i = 0;
    while (rawcmd.at(i) != ':')
        i++;
    cmd.at(2) = rawcmd.substr(i + 1, (rawcmd.size() - i));

    if (cmd[1][0] == '#') {
        for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
            if (it->getName() == lower(cmd.at(1))) {
                for (std::deque<User *>::iterator it2 = it->users.begin(); it2 != it->users.end(); it2++) {
                    if ((*it2)->getFd() == user.fd) {
                        it->sendMsgFromUser(":" + user.nickName + " NOTICE " + cmd.at(1) + " :" + cmd.at(2) + "\r\n", user);
                        return;
                    }
                }
                return;
            }
        }
    } else {
        for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++)
            if (it->getNickName() == cmd.at(1)) {
                it->sendMsg(":" + user.nickName + " NOTICE " + it->nickName + " : " + cmd.at(2) + "\r\n");
                return;
            }
        return;
    }
}

void sendPrivMsg(User &user, std::deque<std::string> &cmd, std::string rawcmd) {
    if (cmd.size() == 1) {
        user.sendMsg("461 " + Server::name + " " + cmd.at(0) + " :Not Enough Parameters\r\n");
        return;
    }
    if (cmd.size() == 2) {
        user.sendMsg(":" + Server::name + " 412 :No text to send\r\n");
        return;
    }
    if (cmd[2][0] != ':') {
        user.sendMsg("ERROR :Invalid message format\r\n");
        return;
    }
    int i = 0;
    while (rawcmd.at(i) != ':')
        i++;
    cmd.at(2) = rawcmd.substr(i + 1, (rawcmd.size() - i));

    if (cmd[1][0] == '#') {
        for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
            if (it->getName() == lower(cmd.at(1))) {
                for (std::deque<User *>::iterator it2 = it->users.begin(); it2 != it->users.end(); it2++) {
                    if ((*it2)->getFd() == user.fd) {
                        it->sendMsgFromUser(":" + user.nickName + " PRIVMSG " + cmd.at(1) + " :" + cmd.at(2) + "\r\n", user);
                        return;
                    }
                }
                user.sendMsg(":" + Server::name + " 404 " + cmd.at(1) + ": Cannot send to CHANNEL\r\n");
                return;
            }
        }
        user.sendMsg(":" + Server::name + " 403 " + cmd.at(1) + ": No such CHANNEL\r\n");
    } else {
        for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++)
            if (it->getNickName() == cmd.at(1)) {
                it->sendMsg(":" + user.nickName + " PRIVMSG " + it->nickName + " : " + cmd.at(2) + "\r\n");
                return;
            }
        user.sendMsg(":" + Server::name + " 401 " + user.nickName + ": No such NICK\r\n");
        return;
    }
}

bool Server::executeCommand(User &user, std::string &cmd) {
    std::cout << BLUE << "[fd: " << user.getFd() << " (" << user.getNickName() << ")] " << RESET << cmd << std::endl;
    std::string stock;
    std::stringstream scmd(cmd);
    std::deque<std::string> cmds;
    if(cmd.empty())
        return false;
    std::cout << "cmd: ";
    while (std::getline(scmd, stock, ' ')) {
        cmds.push_back(stock);
        std::cout << stock << " ";
        stock.clear();
    }
    std::cout << std::endl;

    if (cmds.at(0) == "CAP")
        return false;

    else if (cmds.at(0) == "QUIT") {
        user.closeConnection();
        return false;
    }

    else if (cmds.at(0) == "PASS") 
    {
        if (cmds.size() != 2) {
            user.sendMsg("461 " + Server::name + " " + cmds.at(0) + " :Not Enough Parameters\r\n");
            return false;
        }
        if (user.getIsAuth() == true) {
            user.sendMsg("462 " + Server::name + " :Unauthorized command (already registered)\r\n");
            return false;
        }
        if (cmds.at(1) == Server::password)
            user.setIsAuth(true);
        else {
            user.sendMsg("464 " + Server::name + " :Password incorrect\r\n");
            user.closeConnection();
            return true;
        }
    } 
    else if (!user.getIsAuth()) 
    {
        user.sendMsg(":" + Server::name + " 464 " + user.getNickName() + " :You're no authentify !\r\n");
        user.closeConnection();
        return true;
    }

    else if (cmds.at(0) == "PING")
        user.sendMsg("PONG\r\n");

    else if (cmds.at(0) == "NICK")
        nick(user, cmds);

    else if (cmds.at(0) == "USER" && user.getUserName().empty() && user.getNickName().empty() == false) {
        if (cmds.size() < 2) {
            user.sendMsg("461 " + Server::name + " " + cmds.at(0) + " :Not Enough Parameters\r\n");
            return false;
        }
        user.userName = cmds.at(1);
        std::ifstream file("asset/motd.txt");
        std::string text;
        std::string line;
        while (std::getline(file, line))
            text += MAGENTA + line + RESET + "\n";
        user.sendMsg(":" + Server::name + " 001 " + user.nickName + " :" + text + "\r\n");
    } else if (cmds.at(0) == "PRIVMSG" && user.getIsAuth() == true)
        sendPrivMsg(user, cmds, cmd);
    else if (cmds.at(0) == "NOTICE" && user.getIsAuth() == true)
        sendNoticeMsg(user, cmds, cmd);
    else if (cmds.at(0) == "JOIN" && user.getIsAuth() == true) {
        if (cmds.size() < 2) {
            user.sendMsg(":" + user.getNickName() + " 461 :Not Enough Parameters\r\n");
            return false;
        }
        if (cmds.size() > 3 || cmds[1][0] != '#') {
            user.sendMsg(":" + lower(cmds.at(1)) + " 476 :Bad Channel Mask\r\n");
            return false;
        }
        std::string password = cmds.size() == 3 ? cmds.at(2) : "";
        std::string channelName = cmds.at(1);
        user.joinChannel(lower(channelName), true, password);
    } else if (cmds.at(0) == "PART" && user.getIsAuth() == true)
        user.leaveChannel(cmds);
    else if (cmds.at(0) == "KICK" && user.getIsAuth() == true)
        user.kickChannel(cmds, cmd);
    else if (cmds.at(0) == "TOPIC" && user.getIsAuth() == true)
        user.topic(cmds, cmd);
    else if (cmds.at(0) == "INVITE" && user.getIsAuth() == true)
        user.invite(cmds, cmd);
    else if (cmds.at(0) == "MODE" && user.getIsAuth() == true)
        user.mode(cmds, cmd);
    

    return false;
}

int Server::getServerSocketFd(void) {
    return (Server::fds[0]);
}

void Server::addChannel(std::string const &name, std::string const &operatorName) {
    for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == name) {
            std::cout << RED << "ERROR: Channel already exist" << RESET << std::endl;
            return;
        }
    }
    Server::channels.push_back(Channel(name, operatorName));
}
