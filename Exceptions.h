#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

#include <exception>
#include <string>

class SoldeInsuffisant : public std::exception {
private:
    std::string message;
public:
    SoldeInsuffisant(double solde, double montant){
        message = "Opération impossible : solde insuffisant. Solde actuel: "
                  + std::to_string(solde) + " DH, montant demandé: "
                  + std::to_string(montant) + " DH";
    }
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class CompteIntrouvable : public std::exception {
public:
    const char* what() const noexcept override{
        return "Erreur : Ce compte n'existe pas";
    }
};

#endif // EXCEPTIONS_H_INCLUDED
