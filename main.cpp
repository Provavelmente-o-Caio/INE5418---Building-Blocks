#include <iostream>
#include <string>

#include "app/Agencia.h"
#include "app/Conta.h"
#include "app/BankApplication.h"
#include "net/NetworkNode.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Uso: ./distribuida_snapshot <id_agencia>\n";
        std::cerr << "Exemplo: ./distribuida_snapshot 1\n";
        return 1;
    }

    int idAgencia = std::stoi(argv[1]);

    Agencia agencia(idAgencia);

    if (idAgencia == 1) {
        agencia.addConta(Conta(1, 1000.0, "Alice"));
    } else if (idAgencia == 2) {
        agencia.addConta(Conta(2, 500.0, "Bob"));
    } else if (idAgencia == 3) {
        agencia.addConta(Conta(3, 700.0, "Carol"));
    } else {
        std::cerr << "Agência inválida. Use 1, 2 ou 3.\n";
        return 1;
    }

    NetworkNode node(idAgencia, 5000 + idAgencia);

    node.addNode(1, 5001);
    node.addNode(2, 5002);
    node.addNode(3, 5003);

    BankApplication app(agencia, node);

    node.setMessageHandler([&app](const Message &message) {
        app.handleMensagem(message);
    });

    if (node.start() != 0) {
        std::cerr << "Erro ao iniciar nó " << idAgencia << "\n";
        return 1;
    }

    std::cout << "[AGENCIA " << idAgencia << "] Rodando na porta "
            << 5000 + idAgencia << "\n";

    std::cout << "Comandos:\n";
    std::cout << "  saldo\n";
    std::cout << "  transfer <agencia_destino> <conta_origem> <conta_destino> <valor>\n";
    std::cout << "  ping <agencia_destino>\n";
    std::cout << "  exit\n";

    std::string command;

    while (std::cin >> command) {
        if (command == "exit") {
            break;
        }

        if (command == "saldo") {
            app.imprimirContas();
        } else if (command == "transfer") {
            int agenciaDestino;
            int contaOrigem;
            int contaDestino;
            double valor;

            std::cin >> agenciaDestino >> contaOrigem >> contaDestino >> valor;

            app.transferir(contaOrigem, agenciaDestino, contaDestino, valor);
        } else if (command == "ping") {
            int agenciaDestino;
            std::cin >> agenciaDestino;

            Message ping;
            ping.setType(PING);
            ping.setFrom(idAgencia);
            ping.setTo(agenciaDestino);
            ping.setPayload("PING");

            node.sendTo(agenciaDestino, ping);
        } else {
            std::cout << "Comando desconhecido: " << command << "\n";
        }
    }

    node.stop();

    return 0;
}
