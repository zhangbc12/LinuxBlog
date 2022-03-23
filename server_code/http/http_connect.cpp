# include "http_connect.h"

# include <iostream>

using namespace std;

bool HttpConn::is_ET_;
const char* HttpConn::src_dir_;
atomic<int> HttpConn::client_count_;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    is_close_ = false;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    client_count_++;
    addr_ = addr;
    fd_ = fd;
    read_buffer_.RetrieveAll();
    write_buffer_.RetrieveAll();
    is_close_ = false;
    int count = client_count_;
    LOG_INFO("Client[%d](%s:%d) in, client_count:%d", fd_, GetIP(), GetPort(), count);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if(!is_close_) {
        is_close_ = true;
        client_count_--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, client_count:%d", fd_, GetIP(), GetPort(), (int)client_count_);
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::is_keep_alive() const {
    return request_.is_keep_alive();
}

ssize_t HttpConn::Read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = read_buffer_.ReadFd(fd_, saveErrno);
        if(len <= 0) break;
    } while(is_ET_);
    return len;
}

ssize_t HttpConn::Write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_count_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(ToWriteBytes() == 0) break;
        else if(static_cast<size_t>(len) <= iov_[0].iov_len) {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            write_buffer_.Retrieve(len);
        }
        else {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len > 0) {
                write_buffer_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
    } while(is_ET_ || ToWriteBytes() > 10240);

    return len;
}

bool HttpConn::Process() {
    request_.Init();
    if(read_buffer_.ReadableBytes() <= 0) {
        return false;
    }
    else if(request_.Parse(read_buffer_)) {
        response_.Init(src_dir_, request_.GetPath(), request_.is_keep_alive(), 200);
    }
    else {
        response_.Init(src_dir_, request_.GetPath(), false, 400);
    }

    response_.MakeResponse(write_buffer_);

    // 响应头
    iov_[0].iov_base = const_cast<char*>(write_buffer_.Peek());
    iov_[0].iov_len = write_buffer_.ReadableBytes();
    iov_count_ = 1;

    // 文件
    if(response_.Filelen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.Filelen();
        iov_count_ = 2;
    }
    return true;
}
