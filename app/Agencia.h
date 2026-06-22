//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_AGENCIA_H
#define DISTRIBUIDA_SNAPSHOT_AGENCIA_H
#include <map>
#include "Conta.h"

// Estado local capturado no momento do snapshot
struct EstadoAgencia {
    int idAgencia;
    std::map<int, double> saldos; // contaId -> saldo no momento da captura
};

class Agencia {
public:
    // Construtores e destrutores
    Agencia(int id);

    // Funções de verdade
    void addConta(const Conta &conta);

    void deleteConta(int id);

    void depositar(int id, double valor);

    void sacar(int id, double valor);

    // Captura atômica do estado local para o snapshot de Chandy-Lamport.
    // Deve ser chamada com o mutex da agência já adquirido (ou sem operações
    // concorrentes em andamento), garantindo consistência dos saldos.
    EstadoAgencia captureState() const;

    // Getters e Setters
    int getId() const;

    std::map<int, Conta> getContas();

private:
    int id;
    std::map<int, Conta> contas_locais;
};


#endif //DISTRIBUIDA_SNAPSHOT_AGENCIA_H
