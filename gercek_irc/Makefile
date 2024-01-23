CXX = c++
SRCS = src/main.cpp \
	   src/Server.cpp \
	   src/User.cpp \
	   src/Channel.cpp
OBJS = ${SRCS:.cpp=.o}
NAME = ircserv
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g -I include/

.c.o:
	${CXX} ${CFLAGS} -c $< -o ${<:.c=.o}

${NAME}: ${OBJS}
	${CXX} ${CXXFLAGS} -o ${NAME} ${OBJS}

all: ${OBJS} ${NAME}

clean:
	@rm -f ${OBJS}
	@echo Everything is clean

fclean: clean
	@rm -f ${NAME}

re: fclean all
