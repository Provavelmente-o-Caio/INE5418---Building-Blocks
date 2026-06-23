#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <stdexcept>

#include "app/Agencia.h"
#include "app/Conta.h"
#include "app/BankApplication.h"
#include "net/NetworkNode.h"

static std::vector<std::string> splitCsvLine(const std::string &line, char separator = ';') {
    std::vector<std::string> result;
    std::istringstream stream(line);
    std::string item;

    while (std::getline(stream, item, separator)) {
        result.push_back(item);
    }

    return result;
}

static bool carregarContasDeArquivo(
    Agencia &agencia,
    int idAgencia,
    const std::string &path
) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return false;
    }

    std::string line;
    bool loadedAny = false;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        if (line.empty()) {
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        auto fields = splitCsvLine(line);

        if (fields.size() != 4) {
            std::cerr << "[CONFIG] Linha inválida em " << path
                    << ":" << lineNumber << " -> " << line << "\n";
            continue;
        }

        if (fields[0] == "agencia_id") {
            continue;
        }

        int agenciaArquivo = std::stoi(fields[0]);

        if (agenciaArquivo != idAgencia) {
            continue;
        }

        int contaId = std::stoi(fields[1]);
        std::string titular = fields[2];
        double saldo = std::stod(fields[3]);

        agencia.addConta(Conta(contaId, saldo, titular));
        loadedAny = true;
    }

    return loadedAny;
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

    std::cout << "  criar_conta <agencia_destino> <id_conta> <titular> <saldo>\n";
    std::cout << "      Cria uma conta em uma agência.\n";
    std::cout << "      Exemplo: criar_conta 2 21 Daniel 1000\n\n";

    std::cout << "  apagar_conta <agencia_destino> <id_conta>\n";
    std::cout << "      Apaga uma conta em uma agência.\n";
    std::cout << "      Exemplo: apagar_conta 2 21\n\n";

    std::cout << "  demo_transfer\n";
    std::cout << "      Executa uma transferência de demonstração.\n\n";

    std::cout << "  demo_ping\n";
    std::cout << "      Envia PING para todos os peers em sequência.\n\n";

    std::cout << "  snapshot\n";
    std::cout << "      Inicia snapshot distribuído de Chandy-Lamport a partir desta agência.\n\n";

    std::cout << "  mutex\n";
    std::cout << "      Placeholder para futura exclusão mútua distribuída.\n\n";

    std::cout << "  --- COMANDOS DE SIMULAÇÃO (Sem Parâmetros) ---\n";
    std::cout << "  sim_concorrencia\n";
    std::cout << "      Dispara múltiplas threads transferindo pequenos valores simultaneamente.\n\n";
    
    std::cout << "  sim_atraso\n";
    std::cout << "      Agenda uma transferência para ocorrer após 5 segundos em background.\n\n";

    std::cout << "  sim_snapshot\n";
    std::cout << "      Gera tráfego contínuo de transferências e dispara o snapshot no meio para forçar captura de mensagens em trânsito.\n\n";

    std::cout << "  sim_snapshot_falha\n";
    std::cout << "      tem que falhar manualmente o terminal alvo.\n\n";

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

static void obterRotasDeSimulacao(int idAgencia, int &agenciaDestino, int &contaOrigem, int &contaDestino) {
    // Roteamento circular: Ag1->Ag2, Ag2->Ag3, Ag3->Ag1
    agenciaDestino = (idAgencia % 3) + 1; 
    contaOrigem = idAgencia;
    contaDestino = agenciaDestino;
}

static void simularConcorrencia(BankApplication &app, int idAgencia) {
    int agenciaDestino, contaOrigem, contaDestino;
    obterRotasDeSimulacao(idAgencia, agenciaDestino, contaOrigem, contaDestino);

    std::cout << "[SIMULAÇÃO] Iniciando concorrência massiva (Ag " << idAgencia << " -> Ag " << agenciaDestino << ")...\n";
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&app, agenciaDestino, contaOrigem, contaDestino]() {
            for (int j = 0; j < 5; j++) {
                app.transferir(contaOrigem, agenciaDestino, contaDestino, 1.00);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }
    std::cout << "[SIMULAÇÃO] Concorrência finalizada.\n";
}

static void simularAtraso(BankApplication &app, int idAgencia) {
    int agenciaDestino, contaOrigem, contaDestino;
    obterRotasDeSimulacao(idAgencia, agenciaDestino, contaOrigem, contaDestino);

    std::cout << "[SIMULAÇÃO] Agendando operação com atraso de 5s (Ag " << idAgencia << " -> Ag " << agenciaDestino << ")...\n";
    
    std::thread([&app, agenciaDestino, contaOrigem, contaDestino]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "\n[SIMULAÇÃO] Disparando transferência atrasada agora!\n";
        app.transferir(contaOrigem, agenciaDestino, contaDestino, 50.00);
    }).detach();
}

static void simularSnapshot(BankApplication &app, int idAgencia) {
    int agenciaDestino, contaOrigem, contaDestino;
    obterRotasDeSimulacao(idAgencia, agenciaDestino, contaOrigem, contaDestino);

    std::cout << "[SIMULAÇÃO] Iniciando teste de snapshot com tráfego pesado (Ag " << idAgencia << " -> Ag " << agenciaDestino << ")...\n";
    std::cout << "[SIMULAÇÃO] O atraso no envio do MARKER garantirá a captura das mensagens pelo canal.\n";
    
    // Thread gerando tráfego contínuo (ruído na rede)
    std::thread ruido([&app, agenciaDestino, contaOrigem, contaDestino]() {
        for (int i = 0; i < 2; i++) {
            app.transferir(contaOrigem, agenciaDestino, contaDestino, 5.00);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Envia a cada 100ms
        }
    });

    // Thread disparando o snapshot no meio do tráfego
    std::thread coordenador([&app]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(40)); // Espera 40ms (no meio das 15 transferências)
        std::cout << "\n[SIMULAÇÃO] *** DISPARANDO SNAPSHOT AGORA ***\n";
        app.iniciarSnapshot();
    });

    ruido.detach();
    coordenador.detach();
}

static void simularFalha() {
    std::cout << "[SIMULAÇÃO] Congelando a thread principal do terminal por 20 segundos...\n";
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::cout << "[SIMULAÇÃO] Terminal recuperado.\n";
}

static void sim_snapshot_com_falha(BankApplication &app, int idAgencia, int idAlvoParaFalhar) {
    // Verifica se o alvo é o próprio coordenador (que neste caso é o app)
    if (idAgencia == idAlvoParaFalhar) {
        std::cout << "[ERRO] Você não pode falhar o próprio coordenador neste comando.\n";
        return;
    }

    std::cout << "[SIMULAÇÃO] Agendando falha na Agência " << idAlvoParaFalhar << "...\n";
    
    // Dispara o snapshot normalmente no coordenador (app)
    app.iniciarSnapshot(true);
    
    // Disparo da falha em background
    std::thread([&app, idAlvoParaFalhar]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        
        // Se este fosse o nó alvo, ele falharia. 
        // Como o comando é disparado na agência, você deve disparar 
        // o comando de falha no terminal da agência que você quer derrubar.
        std::cout << "\n[SIMULAÇÃO] !!! FALHA FORÇADA NO NÓ PARTICIPANTE " << idAlvoParaFalhar << " !!!\n";
    }).detach();
}

// ----------------------------------------------

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

        std::string contasPath = "config/contas.csv";

        if (argc >= 3) {
            contasPath = argv[2];
        }

        bool carregouArquivo = carregarContasDeArquivo(agencia, idAgencia, contasPath);

        if (!carregouArquivo) {
            std::cout << "[AGENCIA " << idAgencia << "] "
                    << "Nenhum arquivo de contas carregado.\n";
            throw std::runtime_error("Nenhum arquivo de contas carregado.");
        }
        std::cout << "[AGENCIA " << idAgencia << "] "
                << "Contas carregadas de " << contasPath << "\n";

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

                if (command == "criar_conta") {
                    if (tokens.size() != 5) {
                        std::cerr << "Uso: criar_conta <agencia_destino> <id_conta> <titular> <saldo>\n";
                        continue;
                    }

                    int agenciaDestino = std::stoi(tokens[1]);
                    int idConta = std::stoi(tokens[2]);
                    std::string titular = tokens[3];
                    double saldo = std::stod(tokens[4]);

                    Message criarConta;
                    criarConta.setType(CRIAR_CONTA);
                    criarConta.setFrom(idAgencia);
                    criarConta.setTo(agenciaDestino);

                    std::ostringstream payload;
                    payload << "ACCOUNT_ID=" << idConta << ";"
                            << "TITULAR=" << titular << ";"
                            << "SALDO=" << saldo;

                    criarConta.setPayload(payload.str());

                    int status = node.sendTo(agenciaDestino, criarConta);

                    if (status != 0) {
                        std::cerr << "[AGENCIA " << idAgencia << "] "
                                << "Falha ao enviar solicitação de criação de conta para agência "
                                << agenciaDestino << "\n";
                    }

                    continue;
                }

                if (command == "apagar_conta") {
                    if (tokens.size() != 3) {
                        std::cerr << "Uso: apagar_conta <agencia_destino> <id_conta>\n";
                        continue;
                    }

                    int agenciaDestino = std::stoi(tokens[1]);
                    int idConta = std::stoi(tokens[2]);

                    Message apagarConta;
                    apagarConta.setType(APAGAR_CONTA);
                    apagarConta.setFrom(idAgencia);
                    apagarConta.setTo(agenciaDestino);

                    std::ostringstream payload;
                    payload << "ACCOUNT_ID=" << idConta;

                    apagarConta.setPayload(payload.str());

                    int status = node.sendTo(agenciaDestino, apagarConta);

                    if (status != 0) {
                        std::cerr << "[AGENCIA " << idAgencia << "] "
                                << "Falha ao enviar solicitação de apagar conta para agência "
                                << agenciaDestino << "\n";
                    }

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
                    app.iniciarSnapshot(false); 
                    continue;
                }


                if (command == "mutex") {
                    std::cout << "[AGENCIA " << idAgencia << "] "
                            << "Exclusão mútua distribuída ainda não implementada.\n";
                    continue;
                }

                // --- COMANDOS DE SIMULAÇÃO ---

                if (command == "sim_concorrencia") {
                    simularConcorrencia(app, idAgencia);
                    continue;
                }

                if (command == "sim_atraso") {
                    simularAtraso(app, idAgencia);
                    continue;
                }

                if (command == "sim_snapshot") {
                    std::cout << "[SIMULAÇÃO] Rodando cenário especial de snapshot...\n";
                    
                    simularSnapshot(app, idAgencia); 
                    continue;
                }

                if (command == "sim_falha") {
                    simularFalha();
                    continue;
                }
                if (command == "sim_snapshot_falha") {
                sim_snapshot_com_falha(app, idAgencia, idAgencia+1);
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