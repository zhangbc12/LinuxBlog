# ifndef CONFIG_H
# define CONFIG_H

# include "server/webserver.h"

using namespace std;

class Config {
public:
    Config() {
        port_ = 80;

        trig_mode_ = 1; 

        time_out_ = 30000;

        sql_num_ = 4;

        thread_num_ = 4;

        log_level_ = 1;
    }

    void ParseArg(int argc, char* argv[]) {
        int opt;
        const char *argString = "p:l:m:o:s:t:";

        while((opt = getopt(argc, argv, argString)) != -1) {
            switch (opt)
            {
            case 'p':
                port_ = atoi(optarg);
                break;
            case 'l':
                log_level_ = atoi(optarg);
                break;
            case 'm':
                trig_mode_ = atoi(optarg);
                break;
            case 'o':
                time_out_ = atoi(optarg);
                break;
            case 's':
                sql_num_ = atoi(optarg);
                break;
            case 't':
                thread_num_ = atoi(optarg);
                break;
            default:
                break;
            }
        }
    }

    int port_;

    int trig_mode_;

    int time_out_;

    int sql_num_;

    int thread_num_;

    int sql_port_;

    int log_level_;
};

# endif