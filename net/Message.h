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
    PING,
    PONG
};


class Message {
public:
    Message(TipoMensagem type, std::string from, std::string to, std::string payload);

    ~Message();

    // getters
    const TipoMensagem getType() const;

    const std::string &getFrom() const;

    const std::string &getTo() const;

    const std::string &getPayload() const;

    // setters
    void setType(const TipoMensagem &type);

    void setFrom(const std::string &from);

    void setTo(const std::string &to);

    void setPayload(const std::string &payload);

    //functions
    std::string serialize();

    static Message deserialize(std::string &message);

private:
    TipoMensagem type;
    std::string from;
    std::string to;
    std::string payload;
};


#endif //DISTRIBUIDA_SNAPSHOT_MESSAGE_H
