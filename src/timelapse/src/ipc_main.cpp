#include <iostream>
#include <signal.h>
#include "ninjaipc.h"

using namespace NinjaIPC;
void handle_exit(int sig)
{
    std::cout << "Exiting\n";
    // channel->free();
    exit(0);
}

int main() {
    // signal(SIGINT, handle_exit);
    // signal(SIGTERM, handle_exit);

    auto channel = Channel::make("Readme", sizeof(int));
    while (true) {
        auto i = channel->receive<int>();
        int r = 321;
        channel->reply(r);
        std::cout << "Received " << i << " sent " << r << '\n';
    }
    return 0;
}
