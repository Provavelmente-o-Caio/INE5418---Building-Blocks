//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include <utility>

#include "../net/Message.h"

#include <sstream>

TipoMensagem Message::getType() const {
    return this->type;
}

const int &Message::getFrom() const {
    return this->from;
}

const int &Message::getTo() const {
    return this->to;
}

const std::string &Message::getPayload() const {
    return this->payload;
}

void Message::setFrom(const int &from) {
    this->from = from;
}

void Message::setPayload(const std::string &payload) {
    this->payload = payload;
}

void Message::setTo(const int &to) {
    this->to = to;
}

void Message::setType(const TipoMensagem &type) {
    this->type = type;
}

Message Message::deserialize(const std::string &message) {
    Message result;

    std::istringstream input(message);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        if (line.find(':') == std::string::npos) {
            throw std::runtime_error("Invalid message format");
        }

        std::istringstream input_line(line);
        std::string key, value;
        std::getline(input_line, key, ':');
        std::getline(input_line, value);

        if (key == "type") {
            result.setType(string_to_type(value));
        } else if (key == "from") {
            result.setFrom(std::stoi(value));
        } else if (key == "to") {
            result.setTo(std::stoi(value));
        } else if (key == "payload") {
            result.payload = value;
        } else {
            throw std::runtime_error("Unknown message key");
        }
    }

    return result;
}

std::string Message::serialize() const {
    std::ostringstream output;
    output << "type:" << type_to_string(this->type) << std::endl;
    output << "from:" << this->from << std::endl;
    output << "to:" << this->to << std::endl;
    output << "payload:" << this->payload << std::endl;
    return output.str();
}

std::string Message::type_to_string(const TipoMensagem type) {
    switch (type) {
        case TRANSFERENCIA:
            return "TRANSFERENCIA";
        case RESPOSTA:
            return "RESPOSTA";
        case CRIAR_CONTA:
            return "CRIAR_CONTA";
        case ERRO:
            return "ERRO";
        case PING:
            return "PING";
        case PONG:
            return "PONG";
        default:
            throw std::runtime_error("Unknown message type");
    }
}

TipoMensagem Message::string_to_type(const std::string &type) {
    if (type == "TRANSFERENCIA") {
        return TRANSFERENCIA;
    }
    if (type == "RESPOSTA") {
        return RESPOSTA;
    }
    if (type == "CRIAR_CONTA") {
        return CRIAR_CONTA;
    }
    if (type == "ERRO") {
        return ERRO;
    }
    if (type == "PING") {
        return PING;
    }

    if (type == "PONG") {
        return PONG;
    }

    throw std::runtime_error("Unknown message type");
}
