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
#include <functional>
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
    NetworkNode(int id, int port);

    ~NetworkNode();

    int start();

    int stop();

    int sendTo(int id_receiver, const Message &message) const;

    int broadcast(Message &message);

    void onMessage(const Message &message) const;

    void addNode(const int id, const int port);

    void setMessageHandler(std::function<void(const Message &)> handler);

    // Retorna o mapa de nós conectados (idNó -> porta).
    // Usado pelo BankApplication para saber para quais peers enviar MARKERs
    // e quantos MARKERs esperar receber.
    const std::unordered_map<int, int> &getConnectedNodes() const;

private:
    int id;
    int port;
    std::unordered_map<int, int> connected_nodes;

    int server_fd;
    std::atomic<bool> running;
    std::thread server_thread;

    void server() const;

    std::function<void(const Message &)> messageHandler;
};


#endif //DISTRIBUIDA_SNAPSHOT_NETWORKNODE_H