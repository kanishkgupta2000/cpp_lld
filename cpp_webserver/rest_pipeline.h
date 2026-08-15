#ifndef INCLUDED_REST_PIPELINE
#define INCLUDED_REST_PIPELINE
#include <string>
namespace Rest{
    struct HttpRequest{
        std::string m_path;
        std::string m_method;
        std::string m_version;
    };

    class RestExecutor{
        public:
        RestExecutor(std::string content);
        ~RestExecutor();
        std::string ExecuteQuery();
        private:
        int ParseContent();
        std::string original_content;
        HttpRequest m_httprequest;
    };
}
#endif