//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_MESSAGE_H
#define DISTRIBUIDA_SNAPSHOT_MESSAGE_H
#include <string>

enum TipoMensagem {
    TRANSFERENCIA,
    RESPOSTA,
    CRIAR_CONTA,
    ERRO,
    PING
};


class Message {
public:
    Message() = default;

    const TipoMensagem getType() const;
    const std::string &getFrom() const;
    const std::string &getTo() const;

    void setType(const TipoMensagem &type);
    void setFrom(const std::string &from);
    void setTo(const std::string &to);
private:
    TipoMensagem type;
    std::string from;
    std::string to;
};


#endif //DISTRIBUIDA_SNAPSHOT_MESSAGE_H
