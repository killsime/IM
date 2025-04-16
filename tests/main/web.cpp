#include "Logger.hpp"
#include "test.hpp"
#include <iostream>

int main()
{

    LOG_INFO("started")
    TestAPI::run();

    return 0;
}
