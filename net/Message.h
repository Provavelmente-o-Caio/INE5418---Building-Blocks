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
    Message() = default;

    // getters
    TipoMensagem getType() const;

    const std::string &getFrom() const;

    const std::string &getTo() const;

    const std::string &getPayload() const;

    // setters
    void setType(const TipoMensagem &type);

    void setFrom(const std::string &from);

    void setTo(const std::string &to);

    void setPayload(const std::string &payload);

    //functions
    static Message serialize(std::string &message);

    std::string deserialize();

private:
    TipoMensagem type;
    std::string from;
    std::string to;
    std::string payload;

    static std::string type_to_string(TipoMensagem type);

    static TipoMensagem string_to_type(const std::string &type);
};


#endif //DISTRIBUIDA_SNAPSHOT_MESSAGE_H
