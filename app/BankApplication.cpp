//
// Created by Caio Ferreira Cardoso on 18/06/26.
//

#include "BankApplication.h"

BankApplication::BankApplication(Agencia &agencia, NetworkNode &network)
    : agencia(agencia), network(network) {
}

void BankApplication::handleMensagem(const Message &message) {
    switch (message.getType()) {
        case TRANSFERENCIA:
            handleTransferencia(message);
            break;

        case RESPOSTA:
            handleResposta(message);
            break;

        case CRIAR_CONTA:
            handleCriarConta(message);
            break;

        case PING:
            handlePing(message);
            break;

        case PONG:
            handlePong(message);
            break;

        case ERRO:
            handleErro(message);
            break;

        default:
            std::cerr << "[AGENCIA " << agencia.getId()
                    << "] Tipo de mensagem desconhecido" << std::endl;
            break;
    }
}

void BankApplication::handleTransferencia(const Message &message) const {
    const auto payload = parsePayload(message.getPayload());

    const int toAccount = std::stoi(payload.at("TO_ACCOUNT"));
    const double amount = std::stod(payload.at("AMOUNT"));

    agencia.depositar(toAccount, amount);

    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "Transferência recebida de agência "
            << message.getFrom()
            << ": depositado "
            << amount
            << " na conta "
            << toAccount
            << std::endl;

    Message resposta;
    resposta.setType(RESPOSTA);
    resposta.setFrom(agencia.getId());
    resposta.setTo(message.getFrom());
    resposta.setPayload("STATUS=OK;MESSAGE=TRANSFERENCIA_RECEBIDA");

    network.sendTo(message.getFrom(), resposta);
}

void BankApplication::transferir(
    const int idContaOrigem,
    const int idAgenciaDestino,
    const int idContaDestino,
    const double valor
) {
    agencia.sacar(idContaOrigem, valor);

    if (idAgenciaDestino == agencia.getId()) {
        agencia.depositar(idContaDestino, valor);
        return;
    }

    Message message = criarMensagemTransferencia(
        agencia.getId(),
        idAgenciaDestino,
        idContaOrigem,
        idContaDestino,
        valor
    );

    int status = network.sendTo(idAgenciaDestino, message);

    if (status != 0) {
        agencia.depositar(idContaOrigem, valor);

        std::cerr << "[AGENCIA " << agencia.getId() << "] "
                << "Falha ao enviar transferência. Valor estornado para conta "
                << idContaOrigem << std::endl;
    }
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
    std::cout << "[AGENCIA " << agencia.getId() << "] "
            << "CRIAR_CONTA ainda não implementado: "
            << message.getPayload()
            << std::endl;
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

void BankApplication::imprimirContas() const {
    auto contas = agencia.getContas();

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
