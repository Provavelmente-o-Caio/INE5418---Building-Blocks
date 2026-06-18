//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_AGENCIA_H
#define DISTRIBUIDA_SNAPSHOT_AGENCIA_H
#include <map>
#include <string>
#include "Conta.h"
#include "../net/Message.h"

class Agencia {
public:
    // Construtores e destrutores
    Agencia(int id);

    // Funções de verdade
    void addConta(const Conta &conta);

    void addAgencia(std::string nome, int id);

    void deleteConta(int id);

    void depositar(int id, double valor);

    void sacar(int id, double valor);

    // Getters e Setters
    int getId() const;

    std::map<int, Conta> getContas();

    std::map<int, std::pair<std::string, int> > getAgencias();

private:
    int id;
    std::map<int, Conta> contas_locais;
    std::map<int, std::pair<std::string, int> > agencias;
};


#endif //DISTRIBUIDA_SNAPSHOT_AGENCIA_H
