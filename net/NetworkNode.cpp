//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include "NetworkNode.h"

#include <iostream>
#include <ostream>

NetworkNode::NetworkNode(const int id, const int port) {
    this->id = id;
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
        return -1;
    }

    this->running = true;
    this->server_thread = std::thread(&NetworkNode::server, this);

    std::cout << "Socket criado com sucesso" << std::endl;

    return 0;
}

int NetworkNode::stop() {
    if (!this->running && this->server_fd == -1) {
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

int NetworkNode::sendTo(const int id_receiver, const Message &message) const {
    if (!this->connected_nodes.contains(id_receiver)) {
        std::cerr << "Node desconhecido: " << id_receiver << std::endl;
        return -1;
    }

    int receiver_port = this->connected_nodes.at(id_receiver);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        std::cerr << "Erro ao criar socket cliente" << std::endl;
        return -1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(receiver_port);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        std::cerr << "Endereço inválido" << std::endl;
        close(socket_fd);
        return -1;
    }

    if (connect(socket_fd, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "Erro ao conectar no nó " << id_receiver << std::endl;
        close(socket_fd);
        return -1;
    }

    const std::string raw = message.serialize();

    ssize_t sent = send(socket_fd, raw.c_str(), raw.size(), 0);

    if (sent < 0) {
        std::cerr << "Erro ao enviar mensagem" << std::endl;
        close(socket_fd);
        return -1;
    }

    close(socket_fd);
    return 0;
}

int NetworkNode::broadcast(Message &message) {
    int result = 0;

    for (const auto &[nodeId, port]: connected_nodes) {
        if (nodeId == this->id) {
            continue;
        }

        message.setFrom(this->id);
        message.setTo(nodeId);

        if (sendTo(nodeId, message) != 0) {
            result = -1;
        }
    }

    return result;
}

void NetworkNode::server() const {
    while (this->running) {
        sockaddr client_address{};
        socklen_t client_address_size = sizeof(client_address);

        const int client_fd = accept(this->server_fd, &client_address, &client_address_size);

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
            Message message = Message::deserialize(received_message);
            onMessage(message);
        }

        close(client_fd);
    }
}

void NetworkNode::onMessage(const Message &message) const {
    std::cout << "Mensagem recebida: " << this->id << std::endl;
    std::cout << "Tipo: " << message.serialize() << std::endl;

    if (this->messageHandler) {
        this->messageHandler(message);
    }
}


void NetworkNode::addNode(const int id, const int port) {
    if (!this->connected_nodes.contains(id)) {
        this->connected_nodes[id] = port;
    } else {
        std::cerr << "Node já presente: " << id << std::endl;
    }
}

void NetworkNode::setMessageHandler(std::function<void(const Message &)> handler) {
    this->messageHandler = std::move(handler);
}
