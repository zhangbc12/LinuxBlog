# ifndef HTTP_REQUEST_H
# define HTTP_REQUEST_H

# include <string>
# include <unordered_map>
# include <unordered_set>
# include <mysql/mysql.h>
# include <errno.h>
# include <regex>
# include <algorithm>

# include "../buffer/buffer.h"
# include "../sql_connect_pool/sql_connect_pool_RAII.h"
# include "../record/message.h"
# include "../record/comment.h"
# include "../record/log.h"


using namespace std;

class HttpRequest {
public:
    HttpRequest();

    ~HttpRequest() = default;

    void Init();

    bool Parse(Buffer& buffer);

    string GetPath() const;

    string& GetPath();

    string GetMethod() const;

    string GetVersion() const;

    string GetPost(const string& key) const;

    string GetPost(const char* key) const;

    bool is_keep_alive() const;

private:
    bool ParseRequestLine(const string& line);

    void ParseHeader(const string& line);

    void ParseBody(const string& line);

    void ParsePath();

    void ParsePost();

    void ParsePostContent();

    static bool UserVerify(const string& name, const string& pwd, const string& code, bool is_login);

    static void SaveMessage(const string& name, const string& email, const string& message_content);

    static void SaveComment(const string& blog_name, const string& name, const string& email, const string& message_content);

    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    } state_;

    string method_, path_, version_, body_;

    static const string kMessageFolder;

    unordered_map<string, string> header_;
    unordered_map<string, string> post_;   

    static const unordered_set<string> kDefaultHtml;
    static const unordered_set<string> kArticleTag;
    static const unordered_map<string, int> kHtmlTag;
};

# endif