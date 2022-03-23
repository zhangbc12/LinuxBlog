# ifndef MESSAGE_H
# define MESSAGE_H

# include <string>
# include <sys/time.h>
# include <mutex>
# include <assert.h>
# include <stdio.h>
# include <sys/stat.h>
# include <iostream>

using namespace std;

class Message {
public:
    void Init(const char* path = "./message/", const char* suffix = ".log");

    static Message* GetInstance();

    void WriteMessage(const string& name, const string& email, const string& message_content);

private:
    Message();

    ~Message();

    int today_;

    FILE* fp_;

    const char* path_;

    const char* suffix_;

    char filename_[30];

    mutex mtx_;
};

# endif