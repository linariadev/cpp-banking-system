#ifndef HASHUTILS_H_INCLUDED
#define HASHUTILS_H_INCLUDED

#include <string>
#include <sstream>
#include <iomanip>

class HashUtils {
private:
    // Fonction de hachage simple mais efficace (djb2)
    static unsigned long djb2Hash(const std::string& str) {
        unsigned long hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash;
    }

    // Fonction de hachage additionnelle (sdbm)
    static unsigned long sdbmHash(const std::string& str) {
        unsigned long hash = 0;
        for (char c : str) {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return hash;
    }

public:
    // Combinaison de plusieurs fonctions de hachage pour plus de sécurité
    static std::string hacherMotDePasse(const std::string& motDePasse) {
        // Ajouter un "sel" fixe pour compliquer les attaques
        std::string motDePasseSale = "BanqueSel_TasLin_" + motDePasse + "_FinSel";

        // Calculer plusieurs hashes
        unsigned long hash1 = djb2Hash(motDePasseSale);
        unsigned long hash2 = sdbmHash(motDePasseSale);

        // Combiner les hashes en une chaîne hexadécimale
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        ss << std::setw(16) << hash1;
        ss << std::setw(16) << hash2;

        // Hacher une seconde fois pour plus de sécurité
        std::string premierHash = ss.str();
        unsigned long hash3 = djb2Hash(premierHash);
        unsigned long hash4 = sdbmHash(premierHash);

        ss.str("");
        ss << std::hex << std::setfill('0');
        ss << std::setw(16) << hash3;
        ss << std::setw(16) << hash4;

        return ss.str();
    }

    // Fonction pour vérifier un mot de passe
    static bool verifierMotDePasse(const std::string& motDePasse, const std::string& hashStocke) {
        return hacherMotDePasse(motDePasse) == hashStocke;
    }
};

#endif // HASHUTILS_H_INCLUDED
