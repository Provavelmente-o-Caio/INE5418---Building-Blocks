#include <iostream>
#include "net/NetworkNode.h"


#include <chrono>
#include <thread>

int main() {
    NetworkNode node("A", 5001);

    if (node.start() != 0) {
        return 1;
    }

    std::cout << "Pressione Enter para encerrar..." << std::endl;
    std::cin.get();

    node.stop();

    return 0;
}
