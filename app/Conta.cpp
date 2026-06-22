//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#include "Conta.h"
#include <iostream>
#include <ostream>
#include <utility>

Conta::Conta(const int id, const double saldo, std::string titular) {
    this->id = id;
    this->saldo = saldo;
    this->titular = std::move(titular);
}

void Conta::depositar(const double valor) {
    //this->mtx.lock();
    this->saldo += valor;
    //this->mtx.unlock();
}

void Conta::sacar(const double valor) {
    //this->mtx.lock();
    this->saldo -= valor;
    //this->mtx.unlock();
}

void Conta::exibirHistorico() {
    for (auto &[timestamp, valor]: this->historico) {
        std::cout << timestamp << " - " << valor << std::endl;
    }
}

int Conta::getId() const {
    return this->id;
}

double Conta::getSaldo() const {
    return this->saldo;
}

std::string Conta::getTitular() const {
    return this->titular;
}
