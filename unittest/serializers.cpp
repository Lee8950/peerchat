#include <bits/stdc++.h>
#include "../protocal.hpp"

int main()
{
    assert("TEXT|HELLO" == ecl::serializeCommand(ecl::deserializeCommand("TEXT|HELLO")));
    return 0;
}