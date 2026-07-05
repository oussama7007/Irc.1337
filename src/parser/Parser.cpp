#include "../include/Parser.hpp"
#include <iostream>
#include <sstream>
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
    std::stringstream ss(base_part);
    std::string word;

    // ila kant ":" f lewel donc hadac kaysema prefix ":nick!user@host PRIVMSG #channel :Hello"
    if(ss >> word)
    {
        if(word[0] == ':' ) 
        {
            msg.prefix = word.substr(1);
            if(ss >> word)
                msg.command = word;
                    for (size_t i = 0; i < msg.command.size(); i++)
                        msg.command[i] = toupper((unsigned char)msg.command[i]);
        }
        else 
        {
            msg.command = word;
                    for (size_t i = 0; i < msg.command.size(); i++)
                        msg.command[i] = toupper((unsigned char)msg.command[i]);

        }
        
        // ne9raw dok l words lib9aw as parameters 
        while(ss >> word)
            msg.params.push_back(word);
            
        // flekher ila kan chi text kanzidouh as parameter
        f(has_trailing)
            msg.params.push_back(trailing_part);
            
        return msg;
    }

    return msg;
}