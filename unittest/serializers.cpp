#include <bits/stdc++.h>
#include "../protocal.hpp"

int main()
{
    auto cmd = ecl::deserializeCommand("TEXT|HELLO");
    std::cout << cmd.type << ' ' << cmd.content << std::endl;
    auto str = ecl::serializeCommand(cmd);
    std::cout << str << std::endl;
    return 0;
}