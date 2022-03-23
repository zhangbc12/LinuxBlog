# ifndef SQL_CONNECT_RAII_H
# define SQL_CONNECT_RAII_H

# include "sql_connect_pool.h"

class ConnectPoolRAII {
public:
    ConnectPoolRAII(MYSQL** conn, ConnectPool* conn_pool) {
        assert(conn_pool);

        *conn = conn_pool->GetConnection();
        conn_RAII_ = *conn;
        conn_pool_RAII_ = conn_pool;
    }

    ~ConnectPoolRAII() {
        conn_pool_RAII_->ReleaseConn(conn_RAII_);
    }

private:
    MYSQL* conn_RAII_;
    ConnectPool* conn_pool_RAII_;
};

# endif