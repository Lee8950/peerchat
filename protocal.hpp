#ifndef ECL_CHAT_PROTOCAL_HPP
#define ECL_CHAT_PROTOCAL_HPP 1

#include <string>

/**
 * @version 0.0.1: Big shoutout to liuzn5@mail2.sysu.edu.cn for contributing these tools.
 * Made some small changes to match the project's coding
*/

namespace ecl {

enum commandType
{
    TEXT,
    REG,
    LOGIN,
    DELETE,
};

struct command
{
    commandType type;
    std::string content;
};

std::string serializeCommand(const command &command)
{
    // 序列化用于通过网络发送的命令
    std::string serializedCommand;
    serializedCommand += std::to_string(static_cast<int>(command.type)) + "|";
    serializedCommand += command.content;
    return serializedCommand;
}

command deserializeCommand(const std::string &data)
{
    // 将接收到的数据反序列化为命令
    size_t delimiterPos = data.find('|');
    if (delimiterPos != std::string::npos)
    {
        command parsedCommand;
        parsedCommand.type = static_cast<commandType>(std::stoi(data.substr(0, delimiterPos)));
        parsedCommand.content = data.substr(delimiterPos + 1);
        return parsedCommand;
    }
    else
    {
        // 处理错误或不完整的数据
        return command{TEXT, "ERROR"};
    }
}

}

#endif