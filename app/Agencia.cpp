//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include "Agencia.h"

#include <utility>

Agencia::Agencia(const int id) {
    this->id = id;
}

int Agencia::getId() const {
    return this->id;
}

const std::map<int, Conta> &Agencia::getContas() const {
    return this->contas_locais;
}

void Agencia::sacar(int id, const double valor) {
    this->contas_locais.at(id).sacar(valor);
}

void Agencia::depositar(int id, const double valor) {
    this->contas_locais.at(id).depositar(valor);
}

void Agencia::addConta(Conta conta) {
    this->contas_locais.emplace(conta.getId(), std::move(conta));
}

void Agencia::deleteConta(int id) {
    this->contas_locais.erase(id);
}

EstadoAgencia Agencia::captureState() const {
    EstadoAgencia estado;
    estado.idAgencia = this->id;

    for (const auto &[idConta, conta]: this->contas_locais) {
        estado.saldos[idConta] = conta.getSaldo();
    }

    return estado;
}
