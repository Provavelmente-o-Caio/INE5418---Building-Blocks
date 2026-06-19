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
    PONG,
    MARKER,          // Chandy-Lamport: inicia/propaga snapshot
    SNAPSHOT_ESTADO  // Envia estado local ao coordenador
};


class Message {
public:
    Message() = default;

    // getters
    TipoMensagem getType() const;

    const int &getFrom() const;

    const int &getTo() const;

    const std::string &getPayload() const;

    // setters
    void setType(const TipoMensagem &type);

    void setFrom(const int &from);

    void setTo(const int &to);

    void setPayload(const std::string &payload);

    //functions
    static Message deserialize(const std::string &message);

    std::string serialize() const;

private:
    TipoMensagem type;
    int from;
    int to;
    std::string payload;

    static std::string type_to_string(TipoMensagem type);

    static TipoMensagem string_to_type(const std::string &type);
};


#endif //DISTRIBUIDA_SNAPSHOT_MESSAGE_H