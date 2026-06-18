//
// Created by Caio Ferreira Cardoso on 18/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H
#define DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H

#include "Agencia.h"
#include "../net/Message.h"
#include "../net/NetworkNode.h"

#include <map>
#include <string>

#include <iostream>
#include <sstream>
#include <stdexcept>


class BankApplication {
public:
    BankApplication(Agencia &agencia, NetworkNode &network);

    void handleMensagem(const Message &message);

    void transferir(
        int idContaOrigem,
        int idAgenciaDestino,
        int idContaDestino,
        double valor
    );

    void imprimirContas() const;

private:
    Agencia &agencia;
    NetworkNode &network;

    void handleTransferencia(const Message &message) const;

    void handleResposta(const Message &message) const;

    void handleCriarConta(const Message &message) const;

    void handlePing(const Message &message) const;

    void handlePong(const Message &message) const;

    void handleErro(const Message &message) const;

    static Message criarMensagemTransferencia(
        int idAgenciaOrigem,
        int idAgenciaDestino,
        int idContaOrigem,
        int idContaDestino,
        double valor
    );

    static std::map<std::string, std::string> parsePayload(const std::string &payload);
};


#endif //DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H
