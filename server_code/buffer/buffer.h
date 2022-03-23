# ifndef BUFFER_H
# define BUFFER_H

# include <vector>
# include <algorithm>
# include <cstring>
# include <string>
# include <assert.h>
# include <unistd.h>
# include <atomic>
# include <sys/uio.h>

using namespace std;

class Buffer {
public:
    Buffer(int init_size = 1024);

    ~Buffer();

    int WritableBytes() const;
    int ReadableBytes() const;
    
    const char* Peek() const;
    void EnsureWritable(int len);
    void HasWritten(int len);

    void Retrieve(int len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    string RetrieveAllToStr();

    const char* BeginPtr() const;
    char* BeginPtr();

    void MakeSpace(int len);

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const string& str);
    void Append(const char* str, int len);
    void Append(const void* data, int len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);

private:
    vector<char> buffer_;
    atomic<int> read_pos_;
    atomic<int> write_pos_;
};

# endif