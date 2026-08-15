#include "rest_pipeline.h"
#include <iostream>
#include <string>
#include <ranges>
#include <string_view>
namespace Rest{
    RestExecutor::RestExecutor(std::string content): original_content(content), m_httprequest()
    {
        ParseContent();
    }

    RestExecutor::~RestExecutor(){}
    
    std::string  RestExecutor::ExecuteQuery()
    {
        // I have m_http request
        return m_httprequest.m_method + " method called with PATH: " + m_httprequest.m_path +  " and version: " + m_httprequest.m_version;
    }
    int RestExecutor::ParseContent()
    {
        // original content converted to HttpRequest
        auto buffer_data_lines = original_content | std::views::split('\n');
        auto first_line = *buffer_data_lines.begin();
        std::string line_0(first_line.begin(), first_line.end());

        auto splitted_line_0 = line_0 | std::views::split(' ');

        auto it = splitted_line_0.begin();

        std::string method((*it).begin(), (*it).end());
        ++it;

        std::string path((*it).begin(), (*it).end());
        ++it;

        std::string version((*it).begin(), (*it).end());


        m_httprequest.m_method = method;
        m_httprequest.m_path = path;
        m_httprequest.m_version = version;

        return 0;
    }
}