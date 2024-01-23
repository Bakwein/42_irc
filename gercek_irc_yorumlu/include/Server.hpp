#pragma once

#include <sstream>
#include <string>

#include "Channel.hpp"
#include "User.hpp"
#include "irc.hpp"

class Server {
   public:
    static void createSocket();
    static void bindSocket();
    static void selectSocket();
    static void newConnection();
    static void readInput(User& user);
    static bool executeCommand(User& user, std::string& cmd);
    static void addChannel(std::string const& name, std::string const& operatorName);

    static int getServerSocketFd();

    static int port;
    static int exited;
    static std::string password;
    static std::string name;

    static std::deque<int> fds; // oluşturulan socket fdlerini tutar
    static std::deque<Channel> channels;
    static std::deque<User> users;

    static sockaddr_in addr;
    static fd_set read_fd_set;
};
