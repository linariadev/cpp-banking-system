#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <limits>
#include "sqlite3.h"
#include "Exceptions.h"
#include "HashUtils.h"
#include <memory>

using namespace std;

// Classe Utilisateur (classe de base)
class Utilisateur {
protected:
    int id;
    string nom;
    string motDePasseHash;

public:
    Utilisateur(int i, string n, string mdp) : id(i), nom(n), motDePasseHash(mdp) {}
    virtual ~Utilisateur() {}

    int getId() const { return id; }
    string getNom() const { return nom; }
    bool verifierMotDePasse(string mdp) const { return HashUtils::verifierMotDePasse(mdp, motDePasseHash); }
    string getMotDePasseHash() const { return motDePasseHash; }
    virtual string getType() const = 0;
};

// Classe Admin (hérite de Utilisateur)
class Admin : public Utilisateur {
public:
    Admin(int i, string n, string mdp) : Utilisateur(i, n, mdp) {}
    string getType() const override { return "ADMIN"; }
};

// Classe Client (hérite de Utilisateur)
class Client : public Utilisateur {
public:
    Client(int i, string n, string mdp) : Utilisateur(i, n, mdp) {}
    string getType() const override { return "CLIENT"; }
};

// Classe Historique (pour gérer SQLite)
// Classe Historique - MODIFIÉE
class Historique {
private:
    sqlite3* db;

    string obtenirDateActuelle() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
        return string(buffer);
    }

public:
    Historique() {
        int rc = sqlite3_open("banque.db", &db);
        if (rc) {
            cerr << "Erreur ouverture base de donnees: " << sqlite3_errmsg(db) << endl;
            db = nullptr;
            return;
        }

        // Créer la table des transactions
        const char* sqlTransactions = "CREATE TABLE IF NOT EXISTS transactions ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "numeroCompte INTEGER NOT NULL,"
                         "date TEXT NOT NULL,"
                         "type TEXT NOT NULL,"
                         "montant REAL NOT NULL,"
                         "soldeApres REAL NOT NULL,"
                         "description TEXT);";

        // Créer la table des comptes
        const char* sqlComptes = "CREATE TABLE IF NOT EXISTS comptes ("
                         "numeroCompte INTEGER PRIMARY KEY,"
                         "nomTitulaire TEXT NOT NULL,"
                         "solde REAL NOT NULL,"
                         "typeCompte TEXT NOT NULL,"
                         "etablissement TEXT,"
                         "decouvertAutorise REAL,"
                         "tauxInteret REAL,"
                         "plafondRetrait REAL);";

        // Créer la table des utilisateurs
        const char* sqlUtilisateurs = "CREATE TABLE IF NOT EXISTS utilisateurs ("
                         "id INTEGER PRIMARY KEY,"
                         "nom TEXT UNIQUE NOT NULL,"
                         "motDePasse TEXT NOT NULL,"
                         "typeUtilisateur TEXT NOT NULL);";

        char* errMsg;
        rc = sqlite3_exec(db, sqlTransactions, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "Erreur creation table transactions: " << errMsg << endl;
            sqlite3_free(errMsg);
        }

        rc = sqlite3_exec(db, sqlComptes, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "Erreur creation table comptes: " << errMsg << endl;
            sqlite3_free(errMsg);
        }

        rc = sqlite3_exec(db, sqlUtilisateurs, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "Erreur creation table utilisateurs: " << errMsg << endl;
            sqlite3_free(errMsg);
        }
    }

    ~Historique() {
        if (db) sqlite3_close(db);
    }

    sqlite3* getDb() { return db; }

    void ajouterTransaction(int numCompte, string type, double montant, double soldeApres, string desc) {
        if (!db) return;

        string sql = "INSERT INTO transactions (numeroCompte, date, type, montant, soldeApres, description) "
                    "VALUES (?, ?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numCompte);
            sqlite3_bind_text(stmt, 2, obtenirDateActuelle().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 4, montant);
            sqlite3_bind_double(stmt, 5, soldeApres);
            sqlite3_bind_text(stmt, 6, desc.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                cerr << "Erreur insertion: " << sqlite3_errmsg(db) << endl;
            }
            sqlite3_finalize(stmt);
        }
    }

    void afficherHistorique(int numCompte, string typeCompte, string nomTitulaire) {
        if (!db) return;

        cout << "\n=== HISTORIQUE DES TRANSACTIONS ===\n";
        cout << "Compte: " << typeCompte << " #" << numCompte << "\n";
        cout << "Titulaire: " << nomTitulaire << "\n\n";

        string sql = "SELECT date, type, montant, soldeApres, description FROM transactions "
                    "WHERE numeroCompte = ? ORDER BY id DESC;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numCompte);

            cout << left << setw(20) << "Date" << setw(12) << "Type"
                 << setw(12) << "Montant" << setw(12) << "Solde" << "Description\n";
            cout << string(75, '-') << "\n";

            bool hasData = false;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                hasData = true;
                cout << left << setw(20) << (const char*)sqlite3_column_text(stmt, 0)
                     << setw(12) << (const char*)sqlite3_column_text(stmt, 1)
                     << setw(12) << fixed << setprecision(2) << sqlite3_column_double(stmt, 2)
                     << setw(12) << sqlite3_column_double(stmt, 3)
                     << (const char*)sqlite3_column_text(stmt, 4) << "\n";
            }

            if (!hasData) {
                cout << "Aucune transaction trouvee.\n";
            }

            sqlite3_finalize(stmt);
        }
    }
};

// Classe de base Compte
class Compte {
protected:
    static int compteurID;
    int numeroCompte;
    string nomTitulaire;
    double solde;
    string typeCompte;
    Historique* historique;

    virtual void sauvegarderEnBase() {
        if (!historique || !historique->getDb()) return;

        sqlite3* db = historique->getDb();
        string sql = "INSERT OR REPLACE INTO comptes (numeroCompte, nomTitulaire, solde, typeCompte) "
                    "VALUES (?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numeroCompte);
            sqlite3_bind_text(stmt, 2, nomTitulaire.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, solde);
            sqlite3_bind_text(stmt, 4, typeCompte.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                cerr << "Erreur sauvegarde compte: " << sqlite3_errmsg(db) << endl;
            }
            sqlite3_finalize(stmt);
        }
    }

public:
    Compte(string nom, string type, Historique* hist)
        : nomTitulaire(nom), solde(0), typeCompte(type), historique(hist) {
        numeroCompte = ++compteurID;
        sauvegarderEnBase();
    }

    Compte(int num, string nom, double s, string type, Historique* hist)
        : numeroCompte(num), nomTitulaire(nom), solde(s), typeCompte(type), historique(hist) {
        if (num > compteurID) compteurID = num;
    }

    virtual ~Compte() {}

    void ajouterTransaction(string type, double montant, string desc) {
        if (historique) {
            historique->ajouterTransaction(numeroCompte, type, montant, solde, desc);
        }
    }

    virtual void depot(double montant) {
        if (montant > 0) {
            solde += montant;
            sauvegarderEnBase();
            ajouterTransaction("DEPOT", montant, "Depot d'argent");
            cout << "Depot de " << montant << " DH effectue avec succes!\n";
        } else {
            cout << "Montant invalide!\n";
        }
    }

    virtual bool retrait(double montant) {
        if (montant <= 0) {
            cout << "Montant invalide!\n";
            return false;
        }
        if (solde >= montant) {
            solde -= montant;
            sauvegarderEnBase();
            ajouterTransaction("RETRAIT", montant, "Retrait d'argent");
            cout << "Retrait de " << montant << " DH effectue avec succes!\n";
            return true;
        }
        throw SoldeInsuffisant(solde, montant);
    }

    virtual void virement(double montant, int numDestination) {
        if (retrait(montant)) {
            ajouterTransaction("VIREMENT", montant, "Virement vers compte #" + to_string(numDestination));
        }
    }

    void afficherHistorique() {
        if (historique) {
            historique->afficherHistorique(numeroCompte, typeCompte, nomTitulaire);
        }
    }

    virtual void afficherSolde() {
        cout << "\n=== SOLDE DU COMPTE ===\n";
        cout << "Type: " << typeCompte << "\n";
        cout << "Numero: " << numeroCompte << "\n";
        cout << "Titulaire: " << nomTitulaire << "\n";
        cout << "Solde actuel: " << fixed << setprecision(2) << solde << " DH\n";
    }

    void afficherInfos() {
        cout << " - " << typeCompte << " #" << numeroCompte
             << " | " << nomTitulaire
             << " | Solde: " << fixed << setprecision(2) << solde << " DH\n";
    }

    void supprimerDeBase() {
        if (!historique || !historique->getDb()) return;

        sqlite3* db = historique->getDb();
        string sql = "DELETE FROM comptes WHERE numeroCompte = ?;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numeroCompte);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    int getNumero() const { return numeroCompte; }
    string getNom() const { return nomTitulaire; }
    string getType() const { return typeCompte; }
    double getSolde() const { return solde; }
    void setSolde(double s) {
        solde = s;
        sauvegarderEnBase();
    }
};

int Compte::compteurID = 1000;

// Classe Courant
class Courant : public Compte {
private:
    double decouvertAutorise;

    void sauvegarderEnBase() {
        if (!historique || !historique->getDb()) return;

        sqlite3* db = historique->getDb();
        string sql = "INSERT OR REPLACE INTO comptes (numeroCompte, nomTitulaire, solde, typeCompte, decouvertAutorise) "
                    "VALUES (?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numeroCompte);
            sqlite3_bind_text(stmt, 2, nomTitulaire.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, solde);
            sqlite3_bind_text(stmt, 4, typeCompte.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 5, decouvertAutorise);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

public:
    Courant(string nom, Historique* hist) : Compte(nom, "COURANT", hist) {
        decouvertAutorise = 3000.0;
        sauvegarderEnBase();
    }

    Courant(int num, string nom, double s, double decouv, Historique* hist)
        : Compte(num, nom, s, "COURANT", hist), decouvertAutorise(decouv) {}

    bool retrait(double montant) override {
        if (montant <= 0) {
            cout << "Montant invalide!\n";
            return false;
        }
        if (solde + decouvertAutorise >= montant) {
            solde -= montant;
            sauvegarderEnBase();
            ajouterTransaction("RETRAIT", montant, "Retrait d'argent");
            cout << "Retrait de " << montant << " DH effectue avec succes!\n";
            if (solde < 0) {
                cout << "Attention: Vous utilisez votre decouvert autorise!\n";
            }
            return true;
        }
        throw SoldeInsuffisant(solde + decouvertAutorise, montant);
    }
};

// Classe Epargne
class Epargne : public Compte {
private:
    double tauxInteret;

    void sauvegarderEnBase() {
        if (!historique || !historique->getDb()) return;

        sqlite3* db = historique->getDb();
        string sql = "INSERT OR REPLACE INTO comptes (numeroCompte, nomTitulaire, solde, typeCompte, tauxInteret) "
                    "VALUES (?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numeroCompte);
            sqlite3_bind_text(stmt, 2, nomTitulaire.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, solde);
            sqlite3_bind_text(stmt, 4, typeCompte.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 5, tauxInteret);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

public:
    Epargne(string nom, Historique* hist) : Compte(nom, "EPARGNE", hist) {
        tauxInteret = 3.5;
        sauvegarderEnBase();
    }

    Epargne(int num, string nom, double s, double taux, Historique* hist)
        : Compte(num, nom, s, "EPARGNE", hist), tauxInteret(taux) {}

    void afficherSolde() override {
        double interets = solde * (tauxInteret / 100);
        double soldeAvecInterets = solde + interets;

        cout << "\n=== SOLDE DU COMPTE EPARGNE ===\n";
        cout << "Numero: " << numeroCompte << "\n";
        cout << "Titulaire: " << nomTitulaire << "\n";
        cout << "Taux d'interet: " << tauxInteret << "%\n";
        cout << "Solde avant interets: " << fixed << setprecision(2) << solde << " DH\n";
        cout << "Interets annuels: " << interets << " DH\n";
        cout << "Solde apres interets: " << soldeAvecInterets << " DH\n";
    }

    void appliquerInterets() {
        double interets = solde * (tauxInteret / 100);
        solde += interets;
        sauvegarderEnBase();
        ajouterTransaction("INTERETS", interets, "Interets annuels appliques");
        cout << "Interets de " << interets << " DH appliques!\n";
    }
};

// Classe Etudiant
class Etudiant : public Compte {
private:
    string etablissement;
    double plafondRetrait;

    void sauvegarderEnBase() {
        if (!historique || !historique->getDb()) return;

        sqlite3* db = historique->getDb();
        string sql = "INSERT OR REPLACE INTO comptes (numeroCompte, nomTitulaire, solde, typeCompte, etablissement, plafondRetrait) "
                    "VALUES (?, ?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, numeroCompte);
            sqlite3_bind_text(stmt, 2, nomTitulaire.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 3, solde);
            sqlite3_bind_text(stmt, 4, typeCompte.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, etablissement.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 6, plafondRetrait);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

public:
    Etudiant(string nom, string etab, Historique* hist)
        : Compte(nom, "ETUDIANT", hist), etablissement(etab) {
        plafondRetrait = 1000.0;
        sauvegarderEnBase();
    }

    Etudiant(int num, string nom, double s, string etab, double plafond, Historique* hist)
        : Compte(num, nom, s, "ETUDIANT", hist), etablissement(etab), plafondRetrait(plafond) {}

    bool retrait(double montant) override {
        if (montant > plafondRetrait) {
            cout << "Depassement du plafond de retrait (" << plafondRetrait << " DH)!\n";
            return false;
        }
        return Compte::retrait(montant);
    }

    void afficherSolde() override {
        Compte::afficherSolde();
        cout << "Etablissement: " << etablissement << "\n";
        cout << "Plafond retrait: " << plafondRetrait << " DH\n";
    }
};

// Classe Banque
class Banque {
private:
    vector<Compte*> comptes;
    vector<Utilisateur*> utilisateurs;
    Historique historique;
    Utilisateur* utilisateurConnecte;

    void viderBuffer() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    void chargerUtilisateurs() {
        sqlite3* db = historique.getDb();
        if (!db) return;

        string sql = "SELECT id, nom, motDePasse, typeUtilisateur FROM utilisateurs;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                string nom = (const char*)sqlite3_column_text(stmt, 1);
                string mdpHash = (const char*)sqlite3_column_text(stmt, 2);
                string type = (const char*)sqlite3_column_text(stmt, 3);

            if (type == "ADMIN") {
                utilisateurs.push_back(new Admin(id, nom, mdpHash));  // Passer le hash directement
            } else {
                utilisateurs.push_back(new Client(id, nom, mdpHash));  // Passer le hash directement
            }
            }
            sqlite3_finalize(stmt);
        }
    }

    void chargerComptes() {
        sqlite3* db = historique.getDb();
        if (!db) return;

        string sql = "SELECT numeroCompte, nomTitulaire, solde, typeCompte, etablissement, "
                    "decouvertAutorise, tauxInteret, plafondRetrait FROM comptes;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int num = sqlite3_column_int(stmt, 0);
                string nom = (const char*)sqlite3_column_text(stmt, 1);
                double solde = sqlite3_column_double(stmt, 2);
                string type = (const char*)sqlite3_column_text(stmt, 3);

                Compte* compte = nullptr;

                if (type == "COURANT") {
                    double decouvert = sqlite3_column_double(stmt, 5);
                    compte = new Courant(num, nom, solde, decouvert, &historique);
                } else if (type == "EPARGNE") {
                    double taux = sqlite3_column_double(stmt, 6);
                    compte = new Epargne(num, nom, solde, taux, &historique);
                } else if (type == "ETUDIANT") {
                    string etab = (const char*)sqlite3_column_text(stmt, 4);
                    double plafond = sqlite3_column_double(stmt, 7);
                    compte = new Etudiant(num, nom, solde, etab, plafond, &historique);
                }

                if (compte) comptes.push_back(compte);
            }
            sqlite3_finalize(stmt);
        }
    }

void ajouterUtilisateurEnBase(int id, string nom, string mdp, string type) {
    sqlite3* db = historique.getDb();
    if (!db) return;

    // Hacher le mot de passe avant de le stocker
    string mdpHash = HashUtils::hacherMotDePasse(mdp);

    string sql = "INSERT INTO utilisateurs (id, nom, motDePasse, typeUtilisateur) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, nom.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, mdpHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

    void initialiserDonnees(){
        // Charger utilisateurs depuis la base
        chargerUtilisateurs();

        // Si aucun utilisateur, créer admin par défaut
    if (utilisateurs.empty()){
        string mdpHash = HashUtils::hacherMotDePasse("admin123");
        ajouterUtilisateurEnBase(1, "admin", "admin123", "ADMIN");
        utilisateurs.push_back(new Admin(1, "admin", mdpHash));
    }

        // Charger comptes depuis la base
        chargerComptes();
    }

public:
    Banque() : utilisateurConnecte(nullptr) {
        initialiserDonnees();
    }

    ~Banque() {
        for (auto c : comptes) delete c;
        for (auto u : utilisateurs) delete u;
    }

    bool connexion(string nom, string mdp) {
        for (auto u : utilisateurs) {
            if (u->getNom() == nom && u->verifierMotDePasse(mdp)) {
                utilisateurConnecte = u;
                cout << "\nConnexion reussie! Bienvenue " << nom << " (" << u->getType() << ")\n";
                return true;
            }
        }
        cout << "\nNom d'utilisateur ou mot de passe incorrect!\n";
        return false;
    }

    void creerUtilisateur() {
        cout << "\n=== CREATION D'UTILISATEUR CLIENT ===\n";
        string nom, motDePasse, confirmation;
        int id;

        viderBuffer();
        id = utilisateurs.size() + 1;

        cout << "Nom d'utilisateur: ";
        getline(cin, nom);

        for (auto u : utilisateurs) {
            if (u->getNom() == nom) {
                cout << "Erreur: Ce nom d'utilisateur existe deja!\n";
                return;
            }
        }

        do {
            cout << "Mot de passe: ";
            getline(cin, motDePasse);
            cout << "Confirmer le mot de passe: ";
            getline(cin, confirmation);

            if (motDePasse != confirmation) {
                cout << "\nErreur: Les mots de passe ne correspondent pas!\n";
                cout << "Veuillez reessayer.\n\n";
            }
        } while (motDePasse != confirmation);

        // Hacher le mot de passe
        string mdpHash = HashUtils::hacherMotDePasse(motDePasse);

        Client* nouveauClient = new Client(id, nom, mdpHash);
        utilisateurs.push_back(nouveauClient);
        ajouterUtilisateurEnBase(id, nom, motDePasse, "CLIENT");
        cout << "\nUtilisateur client cree avec succes!\n";
        cout << "ID: " << id << "\n";
        cout << "Nom d'utilisateur: " << nom << "\n";
    }

    void creerCompte() {
        int choix;
        string nom, etab;

        cout << "\n=== CREATION DE COMPTE ===\n";
        cout << "Type de compte:\n";
        cout << "1. Compte Courant\n";
        cout << "2. Compte Epargne\n";
        cout << "3. Compte Etudiant\n";
        cout << "Choix: ";
        cin >> choix;
        viderBuffer();

        cout << "Nom du titulaire: ";
        getline(cin, nom);

        Compte* nouveauCompte = nullptr;

        switch (choix) {
            case 1:
                nouveauCompte = new Courant(nom, &historique);
                break;
            case 2:
                nouveauCompte = new Epargne(nom, &historique);
                break;
            case 3:
                cout << "Etablissement: ";
                getline(cin, etab);
                nouveauCompte = new Etudiant(nom, etab, &historique);
                break;
            default:
                cout << "Choix invalide!\n";
                return;
        }

        comptes.push_back(nouveauCompte);
        cout << "\nCompte cree avec succes!\n";
        cout << "Numero de compte: " << nouveauCompte->getNumero() << "\n";
    }

    void supprimerCompte() {
        try {
            int num;
            cout << "\n=== SUPPRESSION DE COMPTE ===\n";
            cout << "Numero de compte a supprimer: ";
            cin >> num;

            for (size_t i = 0; i < comptes.size(); i++) {
                if (comptes[i]->getNumero() == num) {
                    cout << "Suppression du compte " << comptes[i]->getType()
                         << " de " << comptes[i]->getNom() << "...\n";
                    comptes[i]->supprimerDeBase();
                    delete comptes[i];
                    comptes.erase(comptes.begin() + i);
                    cout << "Compte supprime avec succes!\n";
                    return;
                }
            }
            throw CompteIntrouvable();
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        }
    }

    Compte* trouverCompte(int num) {
        for (auto c : comptes) {
            if (c->getNumero() == num) return c;
        }
        throw CompteIntrouvable();
    }

    void effectuerOperation() {
        try {
            int num, choix;
            double montant;

            cout << "\n=== OPERATIONS BANCAIRES ===\n";
            cout << "Numero de compte: ";
            cin >> num;

            Compte* compte = trouverCompte(num);

            cout << "\nOperation:\n";
            cout << "1. Depot\n";
            cout << "2. Retrait\n";
            cout << "Choix: ";
            cin >> choix;

            cout << "Montant: ";
            cin >> montant;

            if (choix == 1) {
                compte->depot(montant);
            } else if (choix == 2) {
                compte->retrait(montant);
            } else {
                cout << "Choix invalide!\n";
            }
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        } catch (const SoldeInsuffisant& e) {
            cout << e.what() << endl;
        }
    }

    void afficherHistoriqueAdmin() {
        try {
            int num;
            cout << "\n=== HISTORIQUE (ADMIN) ===\n";
            cout << "Numero de compte: ";
            cin >> num;

            Compte* compte = trouverCompte(num);
            compte->afficherHistorique();
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        }
    }

    void afficherSoldeClient(int numCompte) {
        try {
            Compte* compte = trouverCompte(numCompte);
            compte->afficherSolde();
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        }
    }

    void afficherMesComptes(int numCompte) {
        try {
            cout << "\n=== MES COMPTES ===\n";
            bool trouve = false;

            Compte* compteActuel = trouverCompte(numCompte);
            string nomTitulaire = compteActuel->getNom();

            for (auto c : comptes) {
                if (c->getNom() == nomTitulaire) {
                    c->afficherInfos();
                    trouve = true;
                }
            }

            if (!trouve) {
                cout << "Aucun compte trouve.\n";
            }
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        }
    }

    void effectuerVirement(int numSource) {
        try {
            int numDest;
            double montant;

            Compte* source = trouverCompte(numSource);

            cout << "\n=== VIREMENT ===\n";
            cout << "Numero compte destinataire: ";
            cin >> numDest;

            Compte* dest = trouverCompte(numDest);

            cout << "Montant a virer: ";
            cin >> montant;

            if (source->retrait(montant)) {
                dest->depot(montant);
                source->ajouterTransaction("VIREMENT", montant, "Virement vers compte #" + to_string(numDest));
                dest->ajouterTransaction("VIREMENT", montant, "Virement du compte #" + to_string(numSource));
                cout << "Virement effectue avec succes!\n";
            }
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        } catch (const SoldeInsuffisant& e) {
            cout << e.what() << endl;
        }
    }

    void menuAdmin() {
        int choix;
        do {
            cout << "\n======= MENU ADMINISTRATEUR =======\n";
            cout << "1. Creer un utilisateur client\n";
            cout << "2. Creer un compte\n";
            cout << "3. Supprimer un compte\n";
            cout << "4. Historique de transactions\n";
            cout << "5. Operations (depot/retrait)\n";
            cout << "0. Deconnexion\n";
            cout << "Choix: ";
            cin >> choix;

            switch (choix) {
                case 1:
                    creerUtilisateur();
                    break;
                case 2:
                    creerCompte();
                    break;
                case 3:
                    supprimerCompte();
                    break;
                case 4:
                    afficherHistoriqueAdmin();
                    break;
                case 5:
                    effectuerOperation();
                    break;
                case 0:
                    cout << "Deconnexion...\n";
                    utilisateurConnecte = nullptr;
                    break;
                default:
                    cout << "Choix invalide!\n";
            }
        } while (choix != 0);
    }

    void menuClient() {
        try {
            int numCompte, choix;

            cout << "\n=== SELECTION DU COMPTE ===\n";
            cout << "Numero de compte: ";
            cin >> numCompte;

            Compte* compte = trouverCompte(numCompte);

            // Vérifier que le compte appartient au client connecté
            if (compte->getNom() != utilisateurConnecte->getNom()) {
                cout << "Ce compte ne vous appartient pas!\n";
                return;
            }

            do {
                cout << "\n======= MENU CLIENT =======\n";
                cout << "Compte #" << numCompte << " - " << compte->getNom() << "\n";
                cout << "1. Solde actuel\n";
                cout << "2. Mes comptes\n";
                cout << "3. Virement\n";
                cout << "4. Historique de transactions\n";
                cout << "0. Retour\n";
                cout << "Choix: ";
                cin >> choix;

                switch (choix) {
                    case 1:
                        afficherSoldeClient(numCompte);
                        break;
                    case 2:
                        afficherMesComptes(numCompte);
                        break;
                    case 3:
                        effectuerVirement(numCompte);
                        break;
                    case 4:
                        compte->afficherHistorique();
                        break;
                    case 0:
                        cout << "Retour au menu principal...\n";
                        break;
                    default:
                        cout << "Choix invalide!\n";
                }
            } while (choix != 0);
        } catch (const CompteIntrouvable& e) {
            cout << e.what() << endl;
        }
    }

    Utilisateur* getUtilisateurConnecte() {
        return utilisateurConnecte;
    }

    void viderBufferPublic() {
        viderBuffer();
    }
};

int main() {
    Banque banque;
    int choix;
    string nom, mdp;

    cout << "=================================\n";
    cout << " SYSTEME DE GESTION BANCAIRE\n";
    cout << "=================================\n";

    do {
        cout << "\n======= MENU PRINCIPAL =======\n";
        cout << "1. Connexion\n";
        cout << "0. Quitter\n";
        cout << "Choix: ";
        cin >> choix;
        banque.viderBufferPublic();

        switch (choix) {
            case 1:
                cout << "\nNom d'utilisateur: ";
                getline(cin, nom);
                cout << "Mot de passe: ";
                getline(cin, mdp);

                if (banque.connexion(nom, mdp)) {
                    if (banque.getUtilisateurConnecte()->getType() == "ADMIN") {
                        banque.menuAdmin();
                    } else {
                        banque.menuClient();
                    }
                }
                break;
            case 0:
                cout << "Merci d'avoir utilise notre systeme!\n";
                break;
            default:
                cout << "Choix invalide!\n";
        }
    } while (choix != 0);

    return 0;
}
