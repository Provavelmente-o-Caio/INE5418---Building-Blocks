//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include "NetworkNode.h"

#include <iostream>
#include <ostream>
#include <utility>

NetworkNode::NetworkNode(std::string id, const int port) {
    this->id = std::move(id);
    this->port = port;

    this->server_fd = -1;
    this->running = false;
}

NetworkNode::~NetworkNode() {
    this->stop();
}

int NetworkNode::start() {
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (this->server_fd < 0) {
        std::cout << "Erro ao criar socket" << std::endl;
        return -1;
    }

    constexpr int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Erro ao configurar SO_REUSEADDR" << std::endl;
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::cerr << "Erro ao fazer bind na porta " << port << std::endl;
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Erro ao iniciar o servidor" << std::endl;
        close(server_fd);
        server_fd = -1;
    }

    this->running = true;
    this->server_thread = std::thread(&NetworkNode::server, this);

    std::cout << "Socket criado com sucesso" << std::endl;

    return 0;
}

int NetworkNode::stop() {
    if (!this->running && this->server_fd != -1) {
        return 0;
    }
    this->running = false;

    if (this->server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);
        close(this->server_fd);
        this->server_fd = -1;
    }


    if (this->server_thread.joinable()) {
        this->server_thread.join();
    }

    std::cout << "Servidor parado: " << this->id << "." << std::endl;

    return 0;
}

int NetworkNode::sendTo(int id_receiver, Message &message) {
    return 0;
}

int NetworkNode::broadcast(Message &message) {
    return 0;
}

void NetworkNode::server() {
    while (this->running) {
        sockaddr client_address{};
        socklen_t client_address_size = sizeof(client_address);

        int client_fd = accept(this->server_fd, (sockaddr *) &client_address, &client_address_size);

        if (client_fd < 0) {
            if (this->running) {
                std::cout << "Falha ao receber mensagem" << std::endl;
            }
            continue;
        }

        char buffer[1024];

        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string received_message(buffer, static_cast<size_t>(bytes_read));
            onMessage(received_message);
        }

        close(client_fd);
    }
}

void NetworkNode::onMessage(Message &message) {
    std::cout
            <<
            "Mensagem recebida: " << received_message << std::endl;
}
