// #include "../include/ModeCommand.hpp"
// #include "../include/Server.hpp"
// #include "../include/Client.hpp"
// #include "../include/Channel.hpp"
// #include <cstdlib> 

// ModeCommand::ModeCommand() {}
// ModeCommand::~ModeCommand() {}

// void ModeCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
// {
//     if (params.empty())
//     {
//         client.sendMessage(":server 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
//         return;
//     }

//     std::string target = params[0];


//     if (target[0] != '#') return;

//     Channel *channel = server.findChannel(target);
//     if (channel == NULL)
//     {
//         client.sendMessage(":server 403 " + client.getNickname() + " " + target + " :No such channel\r\n");
//         return;
//     }

 
//     if (params.size() == 1)
//     {
//         client.sendMessage(":server 324 " + client.getNickname() + " " + target + " +...\r\n"); 
//         return;
//     }

//     if (!channel->isOperator(&client))
//     {
//         client.sendMessage(":server 482 " + client.getNickname() + " " + target + " :You're not channel operator\r\n");
//         return;
//     }

//     std::string modeString = params[1];
//     bool isPlus = true;
//     size_t paramIndex = 2; 

//     std::string appliedModes = "";
//     std::string appliedParams = "";

//     for (size_t i = 0; i < modeString.length(); ++i)
//     {
//         char c = modeString[i];

//         if (c == '+') { isPlus = true; appliedModes += '+'; }
//         else if (c == '-') { isPlus = false; appliedModes += '-'; }
//         else if (c == 'i') { channel->setInviteOnly(isPlus); appliedModes += 'i'; }
//         else if (c == 't') { channel->setTopicRestricted(isPlus); appliedModes += 't'; }
//         else if (c == 'k') 
//         {
//             if (isPlus && paramIndex < params.size()) 
//             {
//                 channel->setPassword(params[paramIndex]);
//                 appliedModes += 'k';
//                 appliedParams += " " + params[paramIndex];
//                 paramIndex++;
//             }
//             else if (!isPlus)
//             {
//                 channel->setPassword("");
//                 appliedModes += 'k';
//             }
//         }
//         else if (c == 'l') 
//         {
//             if (isPlus && paramIndex < params.size()) 
//             {
//                 channel->setUserLimit(std::atoi(params[paramIndex].c_str()));
//                 appliedModes += 'l';
//                 appliedParams += " " + params[paramIndex];
//                 paramIndex++;
//             }
//             else if (!isPlus) 
//             {
//                 channel->setUserLimit(0);
//                 appliedModes += 'l';
//             }
//         }
//         else if (c == 'o') 
//         {
//             if (paramIndex < params.size())
//             {
//                 std::string targetNick = params[paramIndex++];
//                 Client *targetClient = server.findClientByNick(targetNick);
//                 if (targetClient && channel->isMember(targetClient))
//                 {
//                     if (isPlus) channel->addOperator(targetClient);
//                     else channel->removeOperator(targetClient);
                    
//                     appliedModes += 'o';
//                     appliedParams += " " + targetNick;
//                 }
//                 else
//                 {
//                     client.sendMessage(":server 441 " + client.getNickname() + " " + targetNick + " " + target + " :They aren't on that channel\r\n");
//                 }
//             }
//         }
//         else
//         {
//             client.sendMessage(":server 472 " + client.getNickname() + " " + std::string(1, c) + " :is unknown mode char to me\r\n");
//         }
//     }

//     if (!appliedModes.empty() && appliedModes != "+" && appliedModes != "-")
//     {
//         std::string modeMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost MODE " + target + " " + appliedModes + appliedParams + "\r\n";
//         channel->broadcastMessage(modeMsg);
//         client.sendMessage(modeMsg);
//     }
// }

// Command* createModeCommand() { return new ModeCommand(); }