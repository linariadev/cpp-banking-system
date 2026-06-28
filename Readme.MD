# 🏦 C++ Banking System

### Mini système bancaire sécurisé — Programmation Orientée Objet en C++


## 📌 Description

Application console de gestion bancaire développée en **C++11**. Le système reproduit les opérations essentielles d'une banque : gestion de comptes, opérations bancaires, authentification sécurisée et traçabilité complète des transactions le tout avec persistance via **SQLite3**.

---

## ✨ Fonctionnalités

### 👤 Gestion des utilisateurs
- Authentification par nom d'utilisateur et mot de passe haché
- Deux rôles distincts : **Administrateur** et **Client**
- Création de comptes clients réservée à l'administrateur
- Unicité des noms d'utilisateur vérifiée
- Confirmation du mot de passe à la création

### 🏛️ Gestion des comptes
| Type | Particularité |
|---|---|
| **Courant** | Découvert autorisé de 3 000 DH |
| **Épargne** | Taux d'intérêt annuel de 3,5% |
| **Étudiant** | Plafond de retrait de 1 000 DH par opération |

- Numérotation automatique des comptes (à partir de 1001)
- Création et suppression de comptes (admin uniquement)

### 💸 Opérations bancaires
- Dépôt d'argent
- Retrait avec vérification des règles métier
- Virement entre comptes
- Calcul et application des intérêts (compte épargne)

### 📋 Historique & traçabilité
- Enregistrement automatique de chaque transaction
- Horodatage précis (`YYYY-MM-DD HH:MM:SS`)
- Sauvegarde du solde après chaque opération
- Consultation de l'historique par compte

---

## 📂 Structure du projet

```
cpp-banking-system/
│
├── main.cpp              # Code source principal (toutes les classes)
├── Exceptions.h          # Exceptions personnalisées (SoldeInsuffisant, CompteIntrouvable)
├── HashUtils.h           # Hachage sécurisé des mots de passe (djb2 + sdbm)
├── sqlite3.c             # Code source SQLite3 (amalgamation)
├── sqlite3.h             # Header SQLite3
├── bank_alpha.cbp        # Fichier projet CodeBlocks
│
└── banque.db             # Base de données SQLite (générée automatiquement)
```

---

## ⚙️ Prérequis & Installation

### Prérequis

- **OS :** Windows
- **Compilateur :** [CodeBlocks + MinGW](https://www.codeblocks.org/downloads/) ou MinGW seul
- **Bibliothèque :** `sqlite3.h` + `sqlite3.c` (inclus dans le repo)

### Installation

```bash
# 1. Cloner le repo
git clone https://github.com/linariadev/cpp-banking-system.git
cd cpp-banking-system
```

### Compilation avec CodeBlocks

1. Ouvrir `bank_alpha.cbp`
2. Vérifier que `sqlite3.c` est bien dans le dossier du projet
3. **Build & Run** — `F9`

### Compilation en ligne de commande (MinGW)

```bash
g++ -std=c++11 main.cpp sqlite3.c -o banque.exe
./banque.exe
```

---

## 🚀 Utilisation

### Démarrage

Au premier lancement, un compte administrateur est créé automatiquement :

```
=================================
 SYSTEME DE GESTION BANCAIRE
=================================

======= MENU PRINCIPAL =======
1. Connexion
0. Quitter
Choix:
```

| Identifiant par défaut | Valeur |
|---|---|
| Nom d'utilisateur | `admin` |
| Mot de passe | `admin123` |

---

### Menu Administrateur

```
======= MENU ADMINISTRATEUR =======
1. Creer un utilisateur client
2. Creer un compte
3. Supprimer un compte
4. Historique de transactions
5. Operations (depot/retrait)
0. Deconnexion
```

**Créer un compte client :**
```
=== CREATION D'UTILISATEUR CLIENT ===
Nom d'utilisateur: mohamed
Mot de passe: ********
Confirmer le mot de passe: ********

Utilisateur client cree avec succes!
ID: 2
```

**Créer un compte bancaire :**
```
=== CREATION DE COMPTE ===
Type de compte:
1. Compte Courant
2. Compte Epargne
3. Compte Etudiant
Choix: 1
Nom du titulaire: Mohamed Ameur

Compte cree avec succes!
Numero de compte: 1001
```

---

### Menu Client

```
======= MENU CLIENT =======
Compte #1001 - Mohamed Ameur
1. Solde actuel
2. Mes comptes
3. Virement
4. Historique de transactions
0. Retour
```

**Consulter le solde (compte épargne) :**
```
=== SOLDE DU COMPTE EPARGNE ===
Numero: 1002
Titulaire: Mohamed Ameur
Taux d'interet: 3.5%
Solde avant interets: 10000.00 DH
Interets annuels: 350.00 DH
Solde apres interets: 10350.00 DH
```

**Historique des transactions :**
```
=== HISTORIQUE DES TRANSACTIONS ===
Date                Type        Montant     Solde       Description
---------------------------------------------------------------------------
2026-01-03 14:30:25 RETRAIT     500.00      4500.00     Retrait d'argent
2026-01-03 14:15:10 DEPOT       2000.00     5000.00     Depot d'argent
```

---

### Règles métier

| Règle | Détail |
|---|---|
| Compte Courant | Découvert max : 3 000 DH |
| Compte Épargne | Taux d'intérêt : 3,5% annuel, pas de découvert |
| Compte Étudiant | Plafond par retrait : 1 000 DH, pas de découvert |
| Montants | Doivent être strictement positifs |
| Accès client | Un client ne voit que ses propres comptes |

### Messages d'erreur

| Message | Cause |
|---|---|
| `Nom d'utilisateur ou mot de passe incorrect!` | Identifiants invalides |
| `Erreur : Ce compte n'existe pas` | Numéro de compte introuvable |
| `Solde insuffisant` | Solde (+ découvert si applicable) insuffisant |
| `Depassement du plafond de retrait` | Montant > plafond sur compte étudiant |
| `Ce compte ne vous appartient pas!` | Accès à un compte d'un autre client |

---

## 🗄️ Base de données

La base `banque.db` est créée automatiquement au premier lancement.

### Table `utilisateurs`
| Colonne | Type | Description |
|---|---|---|
| id | INTEGER PK | Identifiant unique |
| nom | TEXT UNIQUE | Nom d'utilisateur |
| motDePasse | TEXT | Mot de passe haché |
| typeUtilisateur | TEXT | `ADMIN` ou `CLIENT` |

### Table `comptes`
| Colonne | Type | Description |
|---|---|---|
| numeroCompte | INTEGER PK | Numéro unique (à partir de 1001) |
| nomTitulaire | TEXT | Nom du titulaire |
| solde | REAL | Solde actuel en DH |
| typeCompte | TEXT | `COURANT`, `EPARGNE` ou `ETUDIANT` |
| decouvertAutorise | REAL | Courant uniquement |
| tauxInteret | REAL | Épargne uniquement |
| etablissement | TEXT | Étudiant uniquement |
| plafondRetrait | REAL | Étudiant uniquement |

### Table `transactions`
| Colonne | Type | Description |
|---|---|---|
| id | INTEGER PK | Auto-incrémenté |
| numeroCompte | INTEGER | Référence au compte |
| date | TEXT | Horodatage ISO |
| type | TEXT | `DEPOT`, `RETRAIT`, `VIREMENT`, `INTERETS` |
| montant | REAL | Montant de l'opération |
| soldeApres | REAL | Solde après l'opération |
| description | TEXT | Description détaillée |

---

## 🔐 Sécurité

Le système implémente un mécanisme de hachage des mots de passe via la classe `HashUtils` :

- Combinaison de deux algorithmes : **djb2** + **sdbm**
- Ajout d'un **sel fixe** pour compliquer les attaques par dictionnaire
- **Double hachage** pour renforcer la résistance
- Production d'un hash hexadécimal de **32 caractères**
- Les mots de passe ne sont **jamais stockés en clair** en base

> ⚠️ Le sel est fixe (non unique par utilisateur). Pour un système en production, un sel aléatoire par utilisateur et bcrypt/Argon2 seraient nécessaires.

---

## 🎓 Concepts POO appliqués

| Concept | Où |
|---|---|
| **Encapsulation** | Attributs `private`/`protected`, accès via getters/setters |
| **Héritage** | `Admin`, `Client` ← `Utilisateur` / `Courant`, `Epargne`, `Etudiant` ← `Compte` |
| **Polymorphisme** | `retrait()`, `afficherSolde()` redéfinis avec `override` |
| **Classe abstraite** | `Utilisateur` avec méthode virtuelle pure `getType()` |
| **Exceptions personnalisées** | `SoldeInsuffisant`, `CompteIntrouvable` héritant de `std::exception` |
| **STL** | `vector<Compte*>`, `vector<Utilisateur*>`, `string`, `iomanip` |
| **Attribut statique** | `Compte::compteurID` pour la numérotation automatique |
| **Composition** | `Banque` contient `Historique` et les collections de pointeurs |

---

## ⚠️ Limites connues

- Interface console uniquement (pas de GUI)
- Pas de gestion multi-utilisateurs simultanés
- Sel de hachage fixe (non unique par utilisateur)
- Pas de tests unitaires automatisés
- La correspondance compte ↔ utilisateur se fait par nom (pas par ID)

---

## 👥 Auteurs

**Lina BEN YASSI** [@linariadev](https://github.com/linariadev)  
**Tasnime FIRKATOUNE**
Note: Projet développé dans le cadre d'un cours de Programmation Orientée Objet en C++ (Janvier 2026).
