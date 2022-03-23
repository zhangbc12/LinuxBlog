# ifndef COMMENT_H
# define COMMENT_H

# include <string>
# include <sys/time.h>
# include <mutex>
# include <assert.h>
# include <stdio.h>
# include <sys/stat.h>
# include <iostream>

using namespace std;

class Comment {
public:
    void Init(const char* path = "./comment/", const char* suffix = ".log");

    static Comment* GetInstance();

    void WriteComment(const string& blog_name, const string& name, const string& email, const string& comment_content);

private:
    Comment();

    ~Comment();

    FILE* fp_;

    const char* path_;

    const char* suffix_;

    char filename_[30];

    mutex mtx_;
};

# endif