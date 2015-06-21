CXX=g++
RM=rm -f
DEBUG=-g
CPPFLAGS=-std=c++1y -lpthread -pedantic -Wall -Wextra -Wcast-align -Wcast-qual \
-Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self \
-Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept \
-Woverloaded-virtual -Wredundant-decls -Wshadow \
-Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 \
-Wswitch-default -Wundef -Werror -Wno-unused

SERVER_SRCS=server.cpp
SERVER_OBJS=$(subst .cpp,.o,$(SERVER_SRCS))

CLIENT_SRCS=client.cpp ThreadQueue.cpp
CLIENT_OBJS=$(subst .cpp,.o,$(CLIENT_SRCS))

OBJS=$(SERVER_OBJS) $(CLIENT_OBJS)

all: server client

server: server.o
	$(CXX) -o stringServer $(SERVER_OBJS) -lpthread

client: client.o ThreadQueue.o
	$(CXX) -o stringClient $(CLIENT_OBJS) -lpthread

ThreadQueue.o: ThreadQueue.cpp ThreadQueue.hpp

server.o: server.cpp server.hpp

client.o: client.cpp client.hpp

clean:
	$(RM) $(OBJS)
