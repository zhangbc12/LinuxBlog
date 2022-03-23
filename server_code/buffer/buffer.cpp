# include "buffer.h"

using namespace std;

Buffer::Buffer(int init_size) :
    buffer_(init_size),
    read_pos_(0),
    write_pos_(0)
    {}

Buffer::~Buffer() = default;

int Buffer::WritableBytes() const {
    return buffer_.size() - write_pos_;
}

int Buffer::ReadableBytes() const {
    return write_pos_ - read_pos_;
}

const char* Buffer::Peek() const {
    return BeginPtr() + read_pos_;
}

void Buffer::EnsureWritable(int len) {
    if(WritableBytes() < len) {
        MakeSpace(len);
    }

    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(int len) {
    write_pos_ += len;
}

void Buffer::Retrieve(int len) {
    assert(len <= ReadableBytes());

    read_pos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(end >= Peek());

    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    read_pos_ = 0;
    write_pos_ = 0;
    bzero(&buffer_[0], buffer_.size());
}

string Buffer::RetrieveAllToStr() {
    string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginPtr() const {
    return &*buffer_.begin();
}

char* Buffer::BeginPtr() {
    return &*buffer_.begin();
}

void Buffer::MakeSpace(int len) {
    if(WritableBytes() + read_pos_ < len) {
        buffer_.resize(write_pos_ + len + 1);
    }
    else {
        int readable = ReadableBytes();
        copy(BeginPtr() + read_pos_, BeginPtr() + write_pos_, BeginPtr());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        
        assert(readable == ReadableBytes());
    }
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr() + write_pos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr() + write_pos_;
}

void Buffer::Append(const string& str) {
    Append(str.data(), str.size());
}

void Buffer::Append(const char* str, int len) {
    assert(str);

    EnsureWritable(len);
    copy(str, str + len, BeginWrite());
    write_pos_ += len;
}

void Buffer::Append(const void* data, int len) {
    assert(data);
    Append(static_cast<const char*> (data), len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];
    struct iovec iov[2];
    const int writable = WritableBytes();

    iov[0].iov_base = BeginPtr() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    
    if(len < 0) {
        *Errno = errno;
    }
    else if(static_cast<int>(len) <= writable) {
        write_pos_ += len;
    }
    else {
        write_pos_ = buffer_.size();
        Append(buff, len - writable);
    }

    return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
    int readable = ReadableBytes();
    ssize_t len = write(fd, Peek(), readable);

    if(len < 0) {
        *Errno = errno;
    }
    else {
        read_pos_ += len;
    }
    return len;
}
