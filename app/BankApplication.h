//
// Created by Caio Ferreira Cardoso on 18/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H
#define DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H

#include "Agencia.h"
#include "../net/Message.h"
#include "../net/NetworkNode.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

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

    // Inicia um snapshot distribuído a partir desta agência (ela é o coordenador).
    void iniciarSnapshot();

    void imprimirContas() const;

private:
    Agencia &agencia;
    NetworkNode &network;

    void handleTransferencia(const Message &message);
    // ----- Estado do snapshot de Chandy-Lamport -----

    // Protege todas as variáveis de snapshot abaixo contra acessos concorrentes.
    std::mutex snapshotMutex;

    // true enquanto um snapshot está em andamento neste nó.
    bool snapshotAtivo{false};

    // true depois que o snapshot mais recente foi finalizado.
    // Usado para ignorar MARKERs duplicados/atrasados do mesmo coordenador.
    bool snapshotConcluido{false};

    // Coordenador do snapshot atual/mais recente.
    int coordenadorId{-1};

    // Estado local capturado no instante do primeiro MARKER recebido (ou ao iniciar).
    EstadoAgencia estadoLocal{};

    // Para cada peer (id da agência), indica se ainda estamos gravando mensagens
    // que chegam por aquele canal (true = gravando, false = MARKER já recebido).
    std::map<int, bool> gravandoCanal;

    // Mensagens de negócio recebidas em cada canal depois de salvar o estado local,
    // mas antes de receber o MARKER daquele canal.
    std::map<int, std::vector<Message>> mensagensCanal;

    // Quantos peers ainda precisam nos enviar um MARKER para fechar o snapshot.
    int marcadoresPendentes{0};

    // ----- Handlers de mensagem -----


    void handleResposta(const Message &message) const;

    void handleCriarConta(const Message &message) const;

    void handlePing(const Message &message) const;

    void handlePong(const Message &message) const;

    void handleErro(const Message &message) const;

    void handleMarker(const Message &message);

    void handleSnapshotEstado(const Message &message);

    // Salva o estado local e inicializa as estruturas de gravação de canal.
    // Deve ser chamado com snapshotMutex já adquirido.
    void salvarEstadoLocal(int idCoordenador);

    // Envia MARKERs para todos os peers.
    void propagarMarkers();

    // Verifica se todos os MARKERs foram recebidos; se sim, finaliza o snapshot.
    // Deve ser chamado com snapshotMutex já adquirido.
    void verificarSnapshotCompleto();

    // Imprime o snapshot global.
    void imprimirSnapshotGlobal(const std::string &estadosSerializados) const;


    static Message criarMensagemTransferencia(
        int idAgenciaOrigem,
        int idAgenciaDestino,
        int idContaOrigem,
        int idContaDestino,
        double valor
    );

    static std::map<std::string, std::string> parsePayload(const std::string &payload);

    int totalPeers() const;
};

#endif //DISTRIBUIDA_SNAPSHOT_BANKAPLICATION_H