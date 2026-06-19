#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "app/Agencia.h"
#include "app/Conta.h"
#include "app/BankApplication.h"
#include "net/NetworkNode.h"

static void carregarContasIniciais(Agencia &agencia, int idAgencia) {
    if (idAgencia == 1) {
        agencia.addConta(Conta(1, 1000.0, "Alice"));
        agencia.addConta(Conta(10, 1500.0, "Ana"));
    } else if (idAgencia == 2) {
        agencia.addConta(Conta(2, 500.0, "Bob"));
        agencia.addConta(Conta(20, 1200.0, "Bruno"));
    } else if (idAgencia == 3) {
        agencia.addConta(Conta(3, 700.0, "Carol"));
        agencia.addConta(Conta(30, 900.0, "Cesar"));
    } else {
        throw std::runtime_error("Agência inválida. Use 1, 2 ou 3.");
    }
}

static void configurarPeers(NetworkNode &node) {
    node.addNode(1, 5001);
    node.addNode(2, 5002);
    node.addNode(3, 5003);
}

static void imprimirAjuda() {
    std::cout << "\nComandos disponíveis:\n";
    std::cout << "  help\n";
    std::cout << "      Mostra esta ajuda.\n\n";

    std::cout << "  saldo\n";
    std::cout << "      Mostra as contas locais da agência.\n\n";

    std::cout << "  ping <agencia_destino>\n";
    std::cout << "      Envia PING para outra agência.\n";
    std::cout << "      Exemplo: ping 2\n\n";

    std::cout << "  broadcast_ping\n";
    std::cout << "      Envia PING para todas as outras agências.\n\n";

    std::cout << "  transfer <agencia_destino> <conta_origem> <conta_destino> <valor>\n";
    std::cout << "      Realiza transferência distribuída.\n";
    std::cout << "      Exemplo: transfer 2 1 2 100\n\n";

    std::cout << "  demo_transfer\n";
    std::cout << "      Executa uma transferência de demonstração, dependendo da agência atual.\n\n";

    std::cout << "  demo_ping\n";
    std::cout << "      Envia PING para todos os peers em sequência.\n\n";

    std::cout << "  snapshot\n";
    std::cout << "      Inicia snapshot distribuído de Chandy-Lamport a partir desta agência.\n";
    std::cout << "      Esta agência será o coordenador e receberá os estados de todas as outras.\n\n";

    std::cout << "  mutex\n";
    std::cout << "      Placeholder para futura exclusão mútua distribuída.\n\n";

    std::cout << "  exit\n";
    std::cout << "      Encerra o processo.\n\n";
}

static std::vector<std::string> splitCommand(const std::string &line) {
    std::vector<std::string> tokens;
    std::istringstream input(line);
    std::string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

static void enviarPing(NetworkNode &node, int idAgencia, int destino) {
    Message ping;
    ping.setType(PING);
    ping.setFrom(idAgencia);
    ping.setTo(destino);
    ping.setPayload("PING");

    int status = node.sendTo(destino, ping);

    if (status != 0) {
        std::cerr << "[AGENCIA " << idAgencia << "] "
                << "Falha ao enviar PING para agência "
                << destino << std::endl;
    }
}

static void enviarBroadcastPing(NetworkNode &node, int idAgencia) {
    Message ping;
    ping.setType(PING);
    ping.setFrom(idAgencia);
    ping.setTo(-1);
    ping.setPayload("BROADCAST_PING");

    int status = node.broadcast(ping);

    if (status != 0) {
        std::cerr << "[AGENCIA " << idAgencia << "] "
                << "Falha parcial ao executar broadcast de PING"
                << std::endl;
    }
}

static void executarDemoPing(NetworkNode &node, int idAgencia) {
    std::cout << "[AGENCIA " << idAgencia << "] Executando demo de PING...\n";

    for (int destino = 1; destino <= 3; destino++) {
        if (destino == idAgencia) {
            continue;
        }

        enviarPing(node, idAgencia, destino);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

static void executarDemoTransferencia(BankApplication &app, int idAgencia) {
    std::cout << "[AGENCIA " << idAgencia << "] Executando demo de transferência...\n";

    if (idAgencia == 1) {
        std::cout << "[DEMO] Agência 1 transfere 100 da conta 1 para conta 2 da agência 2\n";
        app.transferir(1, 2, 2, 100.0);
    } else if (idAgencia == 2) {
        std::cout << "[DEMO] Agência 2 transfere 50 da conta 2 para conta 3 da agência 3\n";
        app.transferir(2, 3, 3, 50.0);
    } else if (idAgencia == 3) {
        std::cout << "[DEMO] Agência 3 transfere 75 da conta 3 para conta 1 da agência 1\n";
        app.transferir(3, 1, 1, 75.0);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Uso: ./distribuida_snapshot <id_agencia>\n";
        std::cerr << "Exemplo:\n";
        std::cerr << "  ./distribuida_snapshot 1\n";
        std::cerr << "  ./distribuida_snapshot 2\n";
        std::cerr << "  ./distribuida_snapshot 3\n";
        return 1;
    }

    int idAgencia = 0;

    try {
        idAgencia = std::stoi(argv[1]);
    } catch (const std::exception &e) {
        std::cerr << "ID de agência inválido: " << argv[1] << "\n";
        return 1;
    }

    try {
        Agencia agencia(idAgencia);
        carregarContasIniciais(agencia, idAgencia);

        NetworkNode node(idAgencia, 5000 + idAgencia);
        configurarPeers(node);

        BankApplication app(agencia, node);

        node.setMessageHandler([&app](const Message &message) {
            app.handleMensagem(message);
        });

        if (node.start() != 0) {
            std::cerr << "[AGENCIA " << idAgencia << "] "
                    << "Erro ao iniciar nó\n";
            return 1;
        }

        std::cout << "[AGENCIA " << idAgencia << "] "
                << "Rodando na porta " << 5000 + idAgencia << "\n";

        imprimirAjuda();

        std::string line;

        while (true) {
            std::cout << "\nagencia-" << idAgencia << "> ";

            if (!std::getline(std::cin, line)) {
                break;
            }

            auto tokens = splitCommand(line);

            if (tokens.empty()) {
                continue;
            }

            const std::string &command = tokens[0];

            try {
                if (command == "exit") {
                    break;
                }

                if (command == "help") {
                    imprimirAjuda();
                    continue;
                }

                if (command == "saldo") {
                    app.imprimirContas();
                    continue;
                }

                if (command == "ping") {
                    if (tokens.size() != 2) {
                        std::cerr << "Uso: ping <agencia_destino>\n";
                        continue;
                    }

                    int destino = std::stoi(tokens[1]);
                    enviarPing(node, idAgencia, destino);
                    continue;
                }

                if (command == "broadcast_ping") {
                    enviarBroadcastPing(node, idAgencia);
                    continue;
                }

                if (command == "transfer") {
                    if (tokens.size() != 5) {
                        std::cerr << "Uso: transfer <agencia_destino> "
                                << "<conta_origem> <conta_destino> <valor>\n";
                        continue;
                    }

                    int agenciaDestino = std::stoi(tokens[1]);
                    int contaOrigem = std::stoi(tokens[2]);
                    int contaDestino = std::stoi(tokens[3]);
                    double valor = std::stod(tokens[4]);

                    app.transferir(contaOrigem, agenciaDestino, contaDestino, valor);
                    continue;
                }

                if (command == "demo_ping") {
                    executarDemoPing(node, idAgencia);
                    continue;
                }

                if (command == "demo_transfer") {
                    executarDemoTransferencia(app, idAgencia);
                    continue;
                }

                if (command == "snapshot") {
                    // Inicia o snapshot distribuído de Chandy-Lamport.
                    // Esta agência será o coordenador: salva seu estado local,
                    // envia MARKERs para todos os peers e aguarda os estados deles.
                    app.iniciarSnapshot();
                    continue;
                }

                if (command == "mutex") {
                    std::cout << "[AGENCIA " << idAgencia << "] "
                            << "Exclusão mútua distribuída ainda não implementada.\n";
                    std::cout <<
                            "Próximo passo: criar MutexManager e mensagens MUTEX_REQUEST/MUTEX_REPLY/MUTEX_RELEASE.\n";
                    continue;
                }

                std::cout << "Comando desconhecido: " << command << "\n";
                std::cout << "Digite 'help' para ver os comandos.\n";
            } catch (const std::exception &e) {
                std::cerr << "[ERRO] " << e.what() << "\n";
            }
        }

        node.stop();

        std::cout << "[AGENCIA " << idAgencia << "] Encerrada.\n";
    } catch (const std::exception &e) {
        std::cerr << "[ERRO FATAL] " << e.what() << "\n";
        return 1;
    }

    return 0;
}