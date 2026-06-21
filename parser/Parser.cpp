#include "include/Parser.hpp"
#include <iostream>

Message Parser::parse(const std::string &raw_line)
{
    Message msg;
    std::string line = raw_line;

    // 7itach most of irc protocol ends with \r\n, o kan 7ido dik \r , \n getline kayhidha automatic  
    if (!line.empty() && line[line.length() - 1] == '\r')
    {
        line.erase(line.length() - 1);
    }
    //ignori empty line
    if (line.empty()) return msg;

   
    std::string base_part = line;
    std::string trailing_part = "";
    bool has_trailing = false;

    //kan9lbo ela : 7itach f irc protocol mesage kayt9sem ela (commands + params)
    size_t trailing_pos = line.find(" :");
    if (trailing_pos != std::string::npos)
    {
        has_trailing = true;
       
        base_part = line.substr(0, trailing_pos);
       
        trailing_part = line.substr(trailing_pos + 2);
    }



    return msg;
}