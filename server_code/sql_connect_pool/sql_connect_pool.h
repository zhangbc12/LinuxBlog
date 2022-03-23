# ifndef SQL_CONNECT_POOL_H
# define SQL_CONNECT_POOL_H

# include <mysql/mysql.h>
# include <thread>
# include <string>
# include <mutex>
# include <queue>
# include <mutex>
# include <semaphore.h>
# include <assert.h>

# include "../record/log.h"

using namespace std;

class ConnectPool {
public:
    void Init(string url, string client, string db_name, string db_pwd, int sql_port, int max_conn);
    // 单例模式
    static ConnectPool *GetInstance();

    MYSQL *GetConnection();

    void ReleaseConn(MYSQL *conn);

    int GetFreeConnNum();

    void ClosePool();


private:
    ConnectPool();

    ~ConnectPool();

    int max_conn_;
    int client_conn_num;
    int free_conn_num_;

    queue<MYSQL*> conn_list_;
    sem_t sem_;
    mutex mtx_;
};

# endif
