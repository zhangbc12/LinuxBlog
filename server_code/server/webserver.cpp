# include "webserver.h"

using namespace std;

Webserver::Webserver(int port, bool opt_linger, int trig_mode, int log_level, int log_queue_size,
              int timeout, int connect_pool_num, int thread_num, int time_step_ms, int time_level,
              int sql_port, const string client, const string db_name, const string db_pwd):
              time_wheel_(new TimeScheduler(time_step_ms, time_level)), thread_pool_(new ThreadPool(thread_num)), epoller_(new Epoller())
    {
    port_ = port;
    open_linger_ = opt_linger;
    timeout_ms_ = timeout;
    is_close_ = false;

    src_dir_ = getcwd(nullptr, 256);
    assert(src_dir_);
    strncat(src_dir_, "/html_pages/", 16);
    HttpConn::client_count_ = 0;
    HttpConn::src_dir_ = src_dir_;
    ConnectPool::GetInstance()->Init("localhost", client, db_name, db_pwd, sql_port, connect_pool_num);
    Message::GetInstance()->Init("./message/", ".log");
    Comment::GetInstance()->Init("./comment/", ".log");
    InitEventMode(trig_mode);
    if(!InitSocket()) {
        is_close_ = true;
    }

    Log::GetInstance()->Init(log_level, "./log/", ".log", log_queue_size);
    if(is_close_) {
        LOG_ERROR("========== Server init error!==========");
    }
    else {
        LOG_INFO("========== Server init ==========");
        LOG_INFO("Port:%d, OpenLinger: %s", port_, open_linger_? "true":"false");
        LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                        (listen_event_ & EPOLLET ? "ET": "LT"),
                        (conn_event_ & EPOLLET ? "ET": "LT"));
        LOG_INFO("LogSys level: %d", log_level);
    }
    time_wheel_->Start();
} 

Webserver::~Webserver() {
    close(listen_fd_);
    is_close_ = true;
    free(src_dir_);
    ConnectPool::GetInstance()->ClosePool();
}

void Webserver::InitEventMode(int trig_mode) {
    listen_event_ = EPOLLRDHUP;
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode)
    {
    case 0:
        break;
    case 1:
        conn_event_ |= EPOLLET;
        break;
    case 2:
        listen_event_ |= EPOLLET;
        break;
    case 3:
        conn_event_ |= EPOLLET;
        listen_event_ |= EPOLLET;
        break;
    default:
        conn_event_ |= EPOLLET;
        listen_event_ |= EPOLLET;
        break;
    }
    HttpConn::is_ET_ = (conn_event_ & EPOLLET);
}

bool Webserver::InitSocket() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        if(port_ != 80) {
            LOG_ERROR("Port:%d error!",  port_);
            return false;
        }
    }

    // htonl,htons将主机字节顺序转换为网络字节顺序
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    // 优雅断开链接
    struct linger opt_linger = {0};
    if(open_linger_) {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if(ret < 0) {
        close(listen_fd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    // 端口复用，只有最后一个套接字正常接受数据
    int optval = 1;
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("Set socket reuse error !");
        close(listen_fd_);
        return false;
    }

    ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listen_fd_);
        return false;
    }

    ret = listen(listen_fd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listen_fd_);
        return false;
    }

    ret = epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen to epoller error!");
        close(listen_fd_);
        return false;
    }

    SetFdNonblock(listen_fd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int Webserver::SetFdNonblock(int fd) {
    assert(fd > 0);
    // O_NONBLOCK 非阻塞I/O;如果read调用没有可读取的数据,或者如果write操作将阻塞,read或write调用返回-1
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void Webserver::Run() {
    int timeMS = -1;
    if(!is_close_) {
        LOG_INFO("========== Server start =========="); 
        cout<<"MyBlog can be visited now!"<<endl;
    }
    while(!is_close_) {
        if(timeout_ms_ > 0) {
            timeMS = time_step_;
        }
        int eventCount = epoller_->Wait(timeMS);
        for(int i = 0; i < eventCount; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);

            if(fd == listen_fd_) {
                DealListen();
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite(&users_[fd]);
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn(&users_[fd]);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void Webserver::SendError(int fd, const char* info) {
    assert(fd > 0);
    send(fd, info, strlen(info), 0);
    close(fd);
}

void Webserver::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if(timeout_ms_ > 0) {
        time_wheel_->CreateTimerTask(fd, timeout_ms_, bind(&Webserver::CloseConn, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | conn_event_);
    SetFdNonblock(fd);
}

void Webserver::DealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listen_fd_, (struct sockaddr*) &addr, &len);
        if(fd <= 0) return;
        else if(HttpConn::client_count_ >= KMaxFd) {
            SendError(fd, "Server is busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient(fd, addr);
    } while(listen_fd_ & EPOLLET);
}

void Webserver::DealRead(HttpConn* client) {
    assert(client);
    ExtentTime(client);
    thread_pool_->EnQueue(bind(&Webserver::OnRead, this, client));
}

void Webserver::DealWrite(HttpConn* client) {
    assert(client);
    ExtentTime(client);
    thread_pool_->EnQueue(bind(&Webserver::OnWrite, this, client));
}

void Webserver::CloseConn(HttpConn* client) {
    assert(client);
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void Webserver::ExtentTime(HttpConn* client) {
    assert(client);
    if(timeout_ms_ > 0) time_wheel_->GetTimer(client->GetFd())->SetRepeated();
}

void Webserver::OnRead(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->Read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void Webserver::OnWrite(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->Write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        if(client->is_keep_alive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}

void Webserver::OnProcess(HttpConn* client) {
    if(client->Process()) {
        epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
    }
    else {
        epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);
    }
}