# include "sql_connect_pool.h"

using namespace std;

ConnectPool::ConnectPool() {
    free_conn_num_ = 0;
    client_conn_num = 0;
}

ConnectPool* ConnectPool::GetInstance() {
    static ConnectPool conn_pool;

    return &conn_pool;
}

void ConnectPool::Init(string url, string client, string db_name, string db_pwd, int sql_port, int max_conn) {
    assert(max_conn > 0);
    for(int i = 0; i < max_conn; i++) {
        MYSQL * conn = nullptr;
        conn = mysql_init(conn);
        if(!conn) {
            LOG_ERROR("MySql init error!");
            assert(conn);
        }

        conn = mysql_real_connect(conn, url.c_str(), client.c_str(), db_pwd.c_str(), db_name.c_str(), sql_port, nullptr, 0);
        if(!conn) {
            LOG_ERROR("MySql connect error!");
            assert(conn);
        }

        conn_list_.push(conn);
        free_conn_num_++;
    }

    max_conn_ = free_conn_num_;

    sem_init(&sem_, 0, free_conn_num_);
}

MYSQL* ConnectPool::GetConnection() {
    if(conn_list_.empty()) return nullptr;

    MYSQL* conn = nullptr;

    sem_wait(&sem_); 
    
    lock_guard<mutex> locker(mtx_);
    conn = conn_list_.front();
    conn_list_.pop();

    free_conn_num_--;
    client_conn_num++;
    
    return conn;
}

void ConnectPool::ReleaseConn(MYSQL* conn) {
    assert(conn);

    lock_guard<mutex> locker(mtx_);

    conn_list_.push(conn);

    free_conn_num_++;
    client_conn_num--;
    
    sem_post(&sem_);
}

int ConnectPool::GetFreeConnNum() {
    lock_guard<mutex> locker(mtx_);

    return this->free_conn_num_;
}

void ConnectPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);

    while(!conn_list_.empty()) {
        auto item = conn_list_.front();
        conn_list_.pop();
        mysql_close(item);
    }

    mysql_library_end();    
}

ConnectPool::~ConnectPool() {
    ClosePool();
}