#include "cloud.h"
#include <iostream>
#include <string>
int main(int argc, char **argv)
{
    if (!get_connect())
    {
        show_info("Get connect false.");
        return -1;
    }
    show_info("Get connect success.");
    show_info("Start run.");
    cloud_run();
    return 0;
}