# ifndef WEBSERVER_H
# define WEBSERVER_H

# include <iostream>
# include <stdio.h>
# include <unordered_map>
# include <fcntl.h>
# include <unistd.h>  // close
# include <assert.h>
# include <errno.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <memory>
# include <string>

# include "../http/http_connect.h"
# include "../time_wheel/time_scheduler.h"
# include "../thread_pool/thread_pool.h"
# include "../sql_connect_pool/sql_connect_pool_RAII.h"
# include "../sql_connect_pool/sql_connect_pool.h"
# include "../epoller/epoller.h"
# include "../record/message.h"
# include "../record/comment.h"
# include "../record/log.h"

using namespace std;


class Webserver {
public:
    Webserver(int port, bool opt_linger, int trig_mode, int log_level, int log_queue_size,
              int timeout, int connect_pool_num, int thread_num, int time_step_ms, int time_level,
              int sql_port, const string client, const string db_name, const string db_pwd); 

    ~Webserver();

    void Run();

private:
    bool InitSocket();

    void InitEventMode(int trig_mode);

    void AddClient(int fd, sockaddr_in addr);

    void DealListen();

    void DealWrite(HttpConn* client);

    void DealRead(HttpConn* client);

    void SendError(int fd, const char* info);

    void ExtentTime(HttpConn* client);

    void CloseConn(HttpConn* clinet);

    void OnRead(HttpConn* client);

    void OnWrite(HttpConn* client);

    void OnProcess(HttpConn* client);

    static int SetFdNonblock(int fd);

    static const int KMaxFd = 65536;

    int port_;
    bool open_linger_;
    int timeout_ms_;
    int time_step_;
    bool is_close_;
    int listen_fd_;
    char* src_dir_;

    uint32_t listen_event_;
    uint32_t conn_event_;

    unique_ptr<TimeScheduler> time_wheel_;
    unique_ptr<ThreadPool> thread_pool_;
    unique_ptr<Epoller> epoller_;
    unordered_map<int, HttpConn> users_;
};

#endif