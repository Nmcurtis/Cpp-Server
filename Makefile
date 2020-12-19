# Compiler Information:
CC = g++ -g -Wall -std=c++17

# Source Files:
SERVER_SOURCE_FILES = server.cpp utility.cpp

# Object File Generation Rules:
SERVER_OBJS = ${SERVER_SOURCE_FILES:.cpp=.o}

# Server Compilation Rule
server: ${SERVER_OBJS}
	${CC} -o $@ $^ -pthread -ldl

# Generic Rules for Compiling a Source File to an Object File
%.o: %.cpp
		${CC} -c $<
%.o: %.cc
		${CC} -c $<

# Clean Rule
clean:
	rm -f ${SERVER_OBJS} server
	rm -rf core.*
