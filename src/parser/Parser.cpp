#include "../include/Parser.hpp"
#include <iostream>
#include <sstream>

//si-hamou
Message Parser::parse(const std::string &raw_line)
{
    Message msg;
    std::string line = raw_line;

    
    if (!line.empty() && line[line.length() - 1] == '\r')
        line.erase(line.length() - 1);
        
    
    if (line.empty()) return msg;

    std::string base_part = line;
    std::string trailing_part = "";
    bool has_trailing = false;

    
    size_t trailing_pos = line.find(" :");
    if (trailing_pos != std::string::npos)
    {
        has_trailing = true;
       
        base_part = line.substr(0, trailing_pos);
       
        trailing_part = line.substr(trailing_pos + 2);
    }
    std::stringstream ss(base_part);
    std::string word;

   
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
        
        while(ss >> word)
            msg.params.push_back(word);
        if(has_trailing)
            msg.params.push_back(trailing_part);
            
        return msg;
    }

    return msg;
}
