//
// Created by Caio Ferreira Cardoso on 18/06/26.
//

#include "BankApplication.h"
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstddef> 

BankApplication::BankApplication(Agencia &agencia, NetworkNode &network)
    : agencia(agencia), network(network), modoSimulado(false) {
}

void BankApplication::handleMensagem(const Message &message) {
    switch (message.getType()) {
        case TRANSFERENCIA: handleTransferencia(message); break;
        case RESPOSTA:      handleResposta(message); break;
        case CRIAR_CONTA:   handleCriarConta(message); break;
        case APAGAR_CONTA:  handleApagarConta(message); break;
        case PING:          handlePing(message); break;
        case PONG:          handlePong(message); break;
        case REQUEST:       handleRequest(message); break;
        case REPLY:         handleReply(message); break;
        case ERRO:          handleErro(message); break;
        case MARKER:        handleMarker(message); break;
        case SNAPSHOT_ESTADO: handleSnapshotEstado(message); break;
        default:
            std::cerr << "[AGENCIA " << agencia.getId() << "] Tipo de mensagem desconhecido" << std::endl;
            break;
    }
}

void BankApplication::handleTransferencia(const Message &message) {
    {
        std::lock_guard<std::mutex> lock(snapshotMutex);
        if (snapshotAtivo && gravandoCanal.count(message.getFrom()) && gravandoCanal.at(message.getFrom())) {
            mensagensCanal[message.getFrom()].push_back(message);
        }
    }

    const auto payload = parsePayload(message.getPayload());
    const int toAccount = std::stoi(payload.at("TO_ACCOUNT"));
    const double amount = std::stod(payload.at("AMOUNT"));

    agencia.depositar(toAccount, amount);

    std::cout << "[AGENCIA " << agencia.getId() << "] Transferência recebida: " << amount << " na conta " << toAccount << std::endl;

    Message resposta;
    resposta.setType(RESPOSTA);
    resposta.setFrom(agencia.getId());
    resposta.setTo(message.getFrom());
    resposta.setPayload("STATUS=OK;MESSAGE=TRANSFERENCIA_RECEBIDA");
    network.sendTo(message.getFrom(), resposta);
}

void BankApplication::transferir(const int idContaOrigem, const int idAgenciaDestino, const int idContaDestino, const double valor) {
    requestCriticalSection();
    agencia.sacar(idContaOrigem, valor);

    if (idAgenciaDestino == agencia.getId()) {
        agencia.depositar(idContaDestino, valor);
        releaseCriticalSection();
        return;
    }

    Message message = criarMensagemTransferencia(agencia.getId(), idAgenciaDestino, idContaOrigem, idContaDestino, valor);
    int status = network.sendTo(idAgenciaDestino, message);

    if (status != 0) {
        agencia.depositar(idContaOrigem, valor);
        std::cerr << "[AGENCIA " << agencia.getId() << "] Falha no envio. Estornado.\n";
    }
    releaseCriticalSection();
}

void BankApplication::iniciarSnapshot(bool modoSimulado) {
    std::lock_guard<std::mutex> lock(snapshotMutex);

    if (snapshotAtivo) {
        std::cout << "[AGENCIA " << agencia.getId() << "] Snapshot já em andamento.\n";
        return;
    }

    this->modoSimulado = modoSimulado;
    this->ativo = true; // Garante que o nó está funcional ao iniciar
    this->inicioSnapshot = std::chrono::steady_clock::now(); // Marca o início para o timeout
    snapshotConcluido = false;

    if (this->modoSimulado) {
        std::cout << "[AGENCIA " << agencia.getId() << "] Iniciando SNAPSHOT EM MODO SIMULADO.\n";
    } else {
        std::cout << "[AGENCIA " << agencia.getId() << "] Iniciando Snapshot Manual.\n";
    }

    salvarEstadoLocal(agencia.getId());
}

void BankApplication::handleMarker(const Message &message) {
    if (!ativo) return;
    std::unique_lock<std::mutex> lock(snapshotMutex);

    const int remetente = message.getFrom();
    const auto payload = parsePayload(message.getPayload());
    const int idCoordenador = std::stoi(payload.at("COORDENADOR"));

    if (!snapshotAtivo) {
        if (snapshotConcluido && idCoordenador == coordenadorId) {
            std::cout << "[AGENCIA " << agencia.getId() << "] "
                    << "MARKER duplicado do snapshot já concluído ignorado.\n";
            return;
        }

        std::cout << "[AGENCIA " << agencia.getId() << "] "
                << "MARKER recebido de " << remetente
                << " — salvando estado e propagando.\n";

        snapshotConcluido = false;
        salvarEstadoLocal(idCoordenador);

    }

    if (gravandoCanal.count(remetente) && gravandoCanal.at(remetente)) {
        gravandoCanal[remetente] = false;
        marcadoresPendentes--;

        std::cout << "[AGENCIA " << agencia.getId() << "] "
                << "Canal de " << remetente
                << " fechado. Pendentes: " << marcadoresPendentes << "\n";
    }

    verificarSnapshotCompleto();
}

void BankApplication::handleSnapshotEstado(const Message &message) {
    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "Estado recebido de agência " << message.getFrom() << ":\n"
            << message.getPayload() << "\n";

    imprimirSnapshotGlobal(message.getPayload());
}

void BankApplication::salvarEstadoLocal(const int idCoordenador) {
    // Pré-condição: snapshotMutex deve estar adquirido pelo chamador.

    snapshotAtivo = true;
    coordenadorId = idCoordenador;
    estadoLocal = agencia.captureState();

    gravandoCanal.clear();
    mensagensCanal.clear();

    auto peers = network.getConnectedNodes();
    marcadoresPendentes = 0;

    for (const auto &[idPeer, porta]: peers) {
        if (idPeer == agencia.getId()) continue;

        gravandoCanal[idPeer] = true;
        mensagensCanal[idPeer] = {};
        marcadoresPendentes++;
    }

    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "Estado local capturado. Aguardando " << marcadoresPendentes
            << " MARKER(s).\n";

    std::cout << "[AGENCIA " << agencia.getId() << "] Saldos no momento do snapshot:\n";
    for (const auto &[idConta, saldo]: estadoLocal.saldos) {
        std::cout << "  Conta " << idConta << ": R$ " << saldo << "\n";
    }

    propagarMarkers();
}
void BankApplication::propagarMarkers() {
    if (!ativo) return;
    auto peers = network.getConnectedNodes();
    std::ostringstream payloadStream;
    payloadStream << "COORDENADOR=" << coordenadorId;
    const std::string payload = payloadStream.str();

    for (const auto &[idPeer, porta]: peers) {
        if (idPeer == agencia.getId()) continue;

        Message marker;
        marker.setType(MARKER);
        marker.setFrom(agencia.getId());
        marker.setTo(idPeer);
        marker.setPayload(payload);

        if (modoSimulado) {
            std::thread([this, idPeer, marker]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                int status = network.sendTo(idPeer, marker);

                if (status != 0) {
                    std::cerr << "[AGENCIA " << agencia.getId() << "] "
                            << "Falha ao enviar MARKER para agência " << idPeer << "\n";
                } else {
                    std::cout << "[AGENCIA " << agencia.getId() << "] "
                            << "MARKER enviado para agência " << idPeer << "\n";
                }
            }).detach();
        } else {
            int status = network.sendTo(idPeer, marker);

        if (status != 0) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] "
                    << "Falha ao enviar MARKER para agência " << idPeer << "\n";
        } else {
            std::cout << "[AGENCIA " << agencia.getId() << "] "
                    << "MARKER enviado para agência " << idPeer << "\n";
        }
        }
    }
}

void BankApplication::verificarSnapshotCompleto() {
    // 1. Verificação de Timeout no Coordenador
    if (snapshotAtivo && coordenadorId == agencia.getId()) {
        auto agora = std::chrono::steady_clock::now();
        auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(agora - inicioSnapshot).count();

        if (duracao > TIMEOUT_MS) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] TIMEOUT! Snapshot global cancelado. Alguma agência falhou ou não respondeu.\n";
            snapshotAtivo = false;
            return; 
        }
    }

    // 2. Verificação de conclusão do protocolo
    if (!snapshotAtivo || marcadoresPendentes > 0) return;

    std::cout << "[AGENCIA " << agencia.getId() << "] Snapshot local completo! Todos os canais fechados.\n";
    
    std::ostringstream resultado;
    resultado << "AGENCIA=" << agencia.getId() << ";";
    for (const auto &[idConta, saldo]: estadoLocal.saldos) 
        resultado << "CONTA_" << idConta << "=" << saldo << ";";
    
    for (const auto &[idPeer, msgs]: mensagensCanal) {
        resultado << "CANAL_" << idPeer << "_MSGS=" << msgs.size() << ";";
        for (size_t i = 0; i < msgs.size(); i++) {
            resultado << "CANAL_" << idPeer << "_MSG_" << i << "=" << msgs[i].getPayload() << ";";
        }
    }

    const std::string estadoStr = resultado.str();
    snapshotAtivo = false;
    snapshotConcluido = true;
    gravandoCanal.clear();
    mensagensCanal.clear();

    if (coordenadorId == agencia.getId()) {
        std::cout << "[AGENCIA " << agencia.getId() << "] === SNAPSHOT GLOBAL FINALIZADO ===\n" << estadoStr << "\n";
    } else {
        Message estadoMsg;
        estadoMsg.setType(SNAPSHOT_ESTADO);
        estadoMsg.setFrom(agencia.getId());
        estadoMsg.setTo(coordenadorId);
        estadoMsg.setPayload(estadoStr);

        if (modoSimulado) {
            std::thread([this, estadoMsg]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                network.sendTo(coordenadorId, estadoMsg);
            }).detach();
        } else {
            network.sendTo(coordenadorId, estadoMsg);
        }
        std::cout << "[AGENCIA " << agencia.getId() << "] Estado enviado ao coordenador (agência " << coordenadorId << ").\n";
    }

    snapshotConcluido = false;
}

void BankApplication::imprimirSnapshotGlobal(const std::string &estadosSerializados) const {
    std::cout << "\n[AGENCIA " << agencia.getId() << "] "
            << "=== ESTADO RECEBIDO DE PARTICIPANTE ===\n"
            << estadosSerializados << "\n"
            << "========================================\n";
}

Message BankApplication::criarMensagemTransferencia(
    const int idAgenciaOrigem,
    const int idAgenciaDestino,
    const int idContaOrigem,
    const int idContaDestino,
    const double valor
) {
    Message message;

    message.setType(TRANSFERENCIA);
    message.setFrom(idAgenciaOrigem);
    message.setTo(idAgenciaDestino);

    std::ostringstream payload;
    payload << "FROM_ACCOUNT=" << idContaOrigem << ";"
            << "TO_ACCOUNT=" << idContaDestino << ";"
            << "AMOUNT=" << valor;

    message.setPayload(payload.str());

    return message;
}

Message BankApplication::criarMensagemRequest(
    const int idAgenciaOrigem,
    const int idAgenciaDestino,
    const int64_t timestamp
) {
    Message message;

    message.setType(REQUEST);
    message.setFrom(idAgenciaOrigem);
    message.setTo(idAgenciaDestino);

    std::ostringstream payload;
    payload << "TIMESTAMP=" << timestamp;
    message.setPayload(payload.str());

    return message;
}

Message BankApplication::criarMensagemReply(
    const int idAgenciaOrigem,
    const int idAgenciaDestino,
    const int64_t timestamp
) {
    Message message;

    message.setType(REPLY);
    message.setFrom(idAgenciaOrigem);
    message.setTo(idAgenciaDestino);

    std::ostringstream payload;
    payload << "TIMESTAMP=" << timestamp;
    message.setPayload(payload.str());

    return message;
}

std::map<std::string, std::string> BankApplication::parsePayload(
    const std::string &payload
) {
    std::map<std::string, std::string> result;

    std::istringstream stream(payload);
    std::string item;

    while (std::getline(stream, item, ';')) {
        std::size_t separator = item.find('=');

        if (separator == std::string::npos) {
            continue;
        }

        std::string key = item.substr(0, separator);
        std::string value = item.substr(separator + 1);

        result[key] = value;
    }

    return result;
}

void BankApplication::handleResposta(const Message &message) const {
    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "Resposta recebida de " << message.getFrom()
            << ": " << message.getPayload()
            << std::endl;
}

void BankApplication::handleCriarConta(const Message &message) const {
    try {
        const auto payload = parsePayload(message.getPayload());

        const int accountId = std::stoi(payload.at("ACCOUNT_ID"));
        const std::string titular = payload.at("TITULAR");
        const double saldoInicial = std::stod(payload.at("SALDO"));

        const auto &contas = agencia.getContas();

        if (contas.contains(accountId)) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] "
                    << "Conta " << accountId << " já existe. "
                    << "Criação recusada.\n";

            Message resposta;
            resposta.setType(ERRO);
            resposta.setFrom(agencia.getId());
            resposta.setTo(message.getFrom());
            resposta.setPayload(
                "STATUS=ERROR;MESSAGE=CONTA_JA_EXISTE;ACCOUNT_ID=" +
                std::to_string(accountId)
            );

            network.sendTo(message.getFrom(), resposta);
            return;
        }

        Conta novaConta(accountId, saldoInicial, titular);
        agencia.addConta(novaConta);

        std::cout << "[AGENCIA " << agencia.getId() << "] "
                << "Conta criada: "
                << "id=" << accountId
                << ", titular=" << titular
                << ", saldo=" << saldoInicial
                << std::endl;

        Message resposta;
        resposta.setType(RESPOSTA);
        resposta.setFrom(agencia.getId());
        resposta.setTo(message.getFrom());
        resposta.setPayload(
            "STATUS=OK;MESSAGE=CONTA_CRIADA;ACCOUNT_ID=" +
            std::to_string(accountId)
        );

        network.sendTo(message.getFrom(), resposta);
    } catch (const std::exception &e) {
        std::cerr << "[AGENCIA " << agencia.getId() << "] "
                << "Erro ao criar conta: "
                << e.what()
                << std::endl;

        Message resposta;
        resposta.setType(ERRO);
        resposta.setFrom(agencia.getId());
        resposta.setTo(message.getFrom());
        resposta.setPayload(
            std::string("STATUS=ERROR;MESSAGE=ERRO_AO_CRIAR_CONTA;DETAIL=") +
            e.what()
        );

        network.sendTo(message.getFrom(), resposta);
    }
}

void BankApplication::handleApagarConta(const Message &message) const {
    try {
        const auto payload = parsePayload(message.getPayload());

        const int accountId = std::stoi(payload.at("ACCOUNT_ID"));

        const auto &contas = agencia.getContas();

        if (!contas.contains(accountId)) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] "
                    << "Conta " << accountId << " não existe. "
                    << "Criação recusada.\n";

            Message resposta;
            resposta.setType(ERRO);
            resposta.setFrom(agencia.getId());
            resposta.setTo(message.getFrom());
            resposta.setPayload(
                "STATUS=ERROR;MESSAGE=CONTA_NAO_EXISTE;ACCOUNT_ID=" +
                std::to_string(accountId)
            );

            network.sendTo(message.getFrom(), resposta);
            return;
        }

        agencia.deleteConta(accountId);

        std::cout << "[AGENCIA " << agencia.getId() << "] "
                << "Conta apagada: "
                << "id=" << accountId
                << std::endl;

        Message resposta;
        resposta.setType(RESPOSTA);
        resposta.setFrom(agencia.getId());
        resposta.setTo(message.getFrom());
        resposta.setPayload(
            "STATUS=OK;MESSAGE=CONTA_APAGADA;ACCOUNT_ID=" +
            std::to_string(accountId)
        );

        network.sendTo(message.getFrom(), resposta);
    } catch (const std::exception &e) {
        std::cerr << "[AGENCIA " << agencia.getId() << "] "
                << "Erro ao apagar conta: "
                << e.what()
                << std::endl;

        Message resposta;
        resposta.setType(ERRO);
        resposta.setFrom(agencia.getId());
        resposta.setTo(message.getFrom());
        resposta.setPayload(
            std::string("STATUS=ERROR;MESSAGE=ERRO_AO_DELETAR_CONTA;DETAIL=") +
            e.what()
        );

        network.sendTo(message.getFrom(), resposta);
    }
}

void BankApplication::handlePing(const Message &message) const {
    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "PING recebido de " << message.getFrom()
            << std::endl;

    Message pong;
    pong.setType(PONG);
    pong.setFrom(agencia.getId());
    pong.setTo(message.getFrom());
    pong.setPayload("PONG");

    network.sendTo(message.getFrom(), pong);
}

void BankApplication::handleErro(const Message &message) const {
    std::cerr << "[AGENCIA " << agencia.getId() << "] "
            << "Erro recebido de " << message.getFrom()
            << ": " << message.getPayload()
            << std::endl;
}

void BankApplication::updateClock(const int64_t timestamp) {
    logicalClock = std::max(logicalClock, timestamp) + 1;
}

bool BankApplication::hasPriorityOver(const int64_t timestampOther, const int nodeOther) const {
    if (requestTimestamp < timestampOther) {
        return true;
    }
    if (requestTimestamp > timestampOther) {
        return false;
    }
    return agencia.getId() < nodeOther;
}

void BankApplication::requestCriticalSection() {
    std::unique_lock<std::mutex> lock(raMutex);

    waitingForReply = true;
    requestTimestamp = ++logicalClock;
    outstandingReplies = totalPeers();
    deferredReplies.clear();

    if (outstandingReplies == 0) {
        return;
    }

    for (const auto &[peerId, peerPort] : network.getConnectedNodes()) {
        if (peerId == agencia.getId()) {
            continue;
        }

        Message request = criarMensagemRequest(agencia.getId(), peerId, requestTimestamp);
        if (network.sendTo(peerId, request) != 0) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] "
                    << "Falha ao enviar REQUEST para agência " << peerId << ". "
                    << "Contando como reply perdida.\n";
            outstandingReplies--;
        }
    }

    raCond.wait(lock, [this] { return outstandingReplies == 0; });
}

void BankApplication::releaseCriticalSection() {
    std::unique_lock<std::mutex> lock(raMutex);

    waitingForReply = false;
    requestTimestamp = -1;

    for (int deferredId : deferredReplies) {
        Message reply = criarMensagemReply(agencia.getId(), deferredId, logicalClock);
        if (network.sendTo(deferredId, reply) != 0) {
            std::cerr << "[AGENCIA " << agencia.getId() << "] "
                    << "Falha ao enviar REPLY adiado para agência " << deferredId << ".\n";
        }
    }

    deferredReplies.clear();
}

void BankApplication::handleRequest(const Message &message) {
    const auto payload = parsePayload(message.getPayload());
    const int64_t timestamp = std::stoll(payload.at("TIMESTAMP"));

    std::unique_lock<std::mutex> lock(raMutex);
    updateClock(timestamp);

    const int requesterId = message.getFrom();
    const bool shouldDefer = waitingForReply && hasPriorityOver(timestamp, requesterId);

    if (shouldDefer) {
        deferredReplies.insert(requesterId);
        std::cout << "[AGENCIA " << agencia.getId() << "] "
                << "Request de " << requesterId << " adiado.\n";
    } else {
        Message reply = criarMensagemReply(agencia.getId(), requesterId, logicalClock);
        network.sendTo(requesterId, reply);
    }
}

void BankApplication::handleReply(const Message &message) {
    const auto payload = parsePayload(message.getPayload());
    const int64_t timestamp = std::stoll(payload.at("TIMESTAMP"));

    std::unique_lock<std::mutex> lock(raMutex);
    updateClock(timestamp);

    if (outstandingReplies > 0) {
        outstandingReplies--;
        if (outstandingReplies == 0) {
            raCond.notify_all();
        }
    }
}

void BankApplication::imprimirContas() const {
    const auto &contas = agencia.getContas();

    std::cout << "[AGENCIA " << agencia.getId() << "] Contas locais:" << std::endl;

    for (const auto &[idConta, conta]: contas) {
        std::cout << "Conta " << idConta
                << " | Titular: " << conta.getTitular()
                << " | Saldo: " << conta.getSaldo()
                << std::endl;
    }
}

void BankApplication::handlePong(const Message &message) const {
    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "PONG recebido de " << message.getFrom()
            << ": " << message.getPayload()
            << std::endl;
}

int BankApplication::totalPeers() const {
    int count = 0;
    for (const auto &[id, porta]: network.getConnectedNodes()) {
        if (id != agencia.getId()) count++;
    }
    return count;
}