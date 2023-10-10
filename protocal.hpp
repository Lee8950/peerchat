#ifndef ECL_CHAT_PROTOCAL_HPP
#define ECL_CHAT_PROTOCAL_HPP 1

#include <string>
#include <unordered_map>

/**
 * @version 0.0.1: Big shoutout to liuzn5@mail2.sysu.edu.cn for contributing these tools.
 * Made some small changes to match the project's coding.
 * @version 0.1.0: Fix bugs. Now it works :)
*/

namespace ecl {

enum commandType
{
    TEXT,
    REG,
    LOGIN,
    DELETE,
};

namespace isolatedGlobalVariables {

std::unordered_map<std::string, ecl::commandType> strToCommandType {
    {"TEXT", commandType::TEXT},
    {"REG", commandType::REG},
    {"LOGIN", commandType::LOGIN},
    {"DELETE", commandType::DELETE}
};
std::unordered_map<ecl::commandType, std::string> commandTypeToStr {
    {commandType::TEXT, "TEXT"},
    {commandType::REG, "REG"},
    {commandType::LOGIN, "LOGIN"},
    {commandType::DELETE, "DELETE"}
};

}

struct commandTypeDict{
    static std::string translate(const ecl::commandType &cmd) {
        return isolatedGlobalVariables::commandTypeToStr[cmd];
    }
    static ecl::commandType translate(const std::string &str) {
        return isolatedGlobalVariables::strToCommandType[str];
    }
    static bool isCommandRegistered(const std::string &str) {
        return isolatedGlobalVariables::strToCommandType.find(str) != isolatedGlobalVariables::strToCommandType.end();
    }
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
        auto tmp = data.substr(0, delimiterPos);
        if(commandTypeDict::translate(tmp))
            return command{TEXT, "Invalid command"};
        parsedCommand.type = commandTypeDict::translate(tmp);
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