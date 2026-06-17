//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
#define DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
#include <string>
#include <unordered_map>


class NetworkNode {
public:
    void start();
    void stop();

    void sendTo();
    void broadcast();
private:
    std::string id;
    int port;
};


#endif //DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
