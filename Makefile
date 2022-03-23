CXX = g++
CFLAGS = -std=c++11 -O2 -Wall -g 

TARGET = server
OBJS = ./server_code/sql_connect_pool/*.cpp ./server_code/time_wheel/*.cpp \
       ./server_code/http/*.cpp server_code/server/*.cpp server_code/epoller/*.cpp \
       server_code/buffer/*.cpp server_code/record/*.cpp server_code/main.cpp 
all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf $(OBJS) $(TARGET)
