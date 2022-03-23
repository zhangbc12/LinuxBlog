# include "server/webserver.h"
# include "config.h"

using namespace std;

int main(int argc, char *argv[]) {
    Config config;
    config.ParseArg(argc, argv);
    
    Webserver server(config.port_, true, config.trig_mode_, config.log_level_, 1024,
              config.time_out_, config.sql_num_, config.thread_num_, 50, 4,
              3306, "root", "myblog", "zbc990812");

    server.Run();
}