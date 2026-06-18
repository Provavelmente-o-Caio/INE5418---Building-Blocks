//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include "Agencia.h"

#include <sstream>
#include <utility>

Agencia::Agencia(const int id) {
    this->id = id;
}

int Agencia::getId() const {
    return this->id;
}

std::map<int, Conta> Agencia::getContas() {
    return this->contas_locais;
}

std::map<int, std::pair<std::string, int> > Agencia::getAgencias() {
    return this->agencias;
}

void Agencia::sacar(int id, const double valor) {
    this->contas_locais.at(id).sacar(valor);
}

void Agencia::depositar(int id, const double valor) {
    this->contas_locais.at(id).depositar(valor);
}

void Agencia::addConta(const Conta &conta) {
    this->contas_locais.insert({conta.getId(), conta});
}
