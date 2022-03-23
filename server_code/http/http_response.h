# ifndef HTTP_RESPONSE_H
# define HTTP_RESPONSE_H

# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <stdio.h>
# include <unordered_map>
# include <string>
# include <assert.h>

# include "../buffer/buffer.h"

using namespace std;

class HttpResponse {
public:
    HttpResponse();

    ~HttpResponse();

    void Init(const string& src_dir_, string& path, bool is_keep_alive = false, int code = -1);

    void MakeResponse(Buffer& buffer);

    void UnmapFile();

    char* File();

    size_t Filelen() const;

    void ErrorContent(Buffer& buffer, string message);

    int Code() const;

private:
    void AddStateLine(Buffer &buffer);

    void AddHeader(Buffer &buffer);

    void AddContent(Buffer &buffer);

    void ErrorHtml();

    string GetFileType();

    int code_;
    bool is_keep_alive_;
    string path_;
    string src_dir_;

    char* mm_file_;
    struct stat mm_file_stat_;

    static const unordered_map<string, string> kSuffixType;
    static const unordered_map<int, string> kCodeStatus;
    static const unordered_map<int, string> kCodePath;
};

# endif