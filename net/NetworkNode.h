//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
#define DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
#include <string>
#include <unordered_map>
#include "Message.h"
#include <atomic>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>

struct NodeAddress {
    std::string id;
    int port;
};


class NetworkNode {
public:
    NetworkNode(std::string id, int port);

    ~NetworkNode();

    int start();

    int stop();

    int sendTo(int id_receiver, Message &message);

    int broadcast(Message &message);

    void onMessage(Message &message);

private:
    std::string id;
    int port;
    std::unordered_map<std::string, int> connected_nodes;

    int server_fd;
    std::atomic<bool> running;
    std::thread server_thread;

    void server() const;
};


#endif //DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H
