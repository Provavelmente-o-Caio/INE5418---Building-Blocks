//
// Created by Caio Ferreira Cardoso on 17/06/26.
//

#ifndef DISTRIBUIDA_SNAPSHOT_CONTA_H
#define DISTRIBUIDA_SNAPSHOT_CONTA_H
#include <ctime>
#include <string>
#include <vector>

struct Transferencia {
    std::time_t timestamp;
    double valor;
};

class Conta {
public:
    Conta(int id, double saldo, std::string titular);
    ~Conta();

    void depositar(double valor);
    void sacar(double valor);
    void transferir(double valor, Conta &destino);
    void exibirHistorico();

    int getId();
    double getSaldo();
    std::string getTitular();
private:
    int id;
    double saldo;
    std::string titular;
    std::vector<Transferencia> historico;
};


#endif //DISTRIBUIDA_SNAPSHOT_CONTA_H
