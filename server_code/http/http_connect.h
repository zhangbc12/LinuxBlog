# ifndef HTTPCONN_H
# define HTTPCONN_H

# include <sys/types.h>
# include <sys/uio.h>   // readv/writev
# include <arpa/inet.h> // sockaddr_in
# include <stdlib.h> // atoi()
# include <errno.h>

# include "../sql_connect_pool/sql_connect_pool_RAII.h"
# include "../buffer/buffer.h"
# include "http_request.h"
# include "http_response.h"

using namespace std;

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void Init(int sock_fd, const sockaddr_in& addr);

    ssize_t Read(int *saveErrno);

    ssize_t Write(int *saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;

    sockaddr_in GetAddr() const;

    bool Process();

    int ToWriteBytes();

    bool is_keep_alive() const;

    static bool is_ET_;

    static const char* src_dir_;

    static atomic<int> client_count_;

private:
    int fd_;
    sockaddr_in addr_;

    bool is_close_;

    int iov_count_;
    struct iovec iov_[2];

    Buffer read_buffer_;
    Buffer write_buffer_;

    HttpRequest request_;
    HttpResponse response_; 
};

# endif