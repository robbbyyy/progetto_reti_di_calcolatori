#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <sys/socket.h>
#include <string.h>



MYSQL *connessione() {
    // Inizializza una nuova connessione MySQL
    MYSQL *connetti = mysql_init(NULL);

    // Se la connessione non è stata inizializzata correttamente, stampa un messaggio di errore, chiude la connessione e termina il programma
    if (connetti == NULL) {
        fprintf(stderr, "Errore nell' inizializzazione mysql.\n");
        mysql_close(connetti);
        exit(1);
    }

    // Prova a stabilire una connessione al server MySQL. Se la connessione fallisce, stampa un messaggio di errore, chiude la connessione e termina il programma
    if (mysql_real_connect(connetti, "127.0.0.1", "root", "ROOT123root__", "universita", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "Errore nella connessione mysql.\n");
        mysql_close(connetti);
        exit(1);
    } else {
        printf("Connessione al database stabilita con successo!\n");
    }


    // Se la connessione è stata stabilita con successo, restituisce l'oggetto MYSQL che rappresenta la connessione
    return connetti;
}



void aggiungi_esame(int connfd) {
    // Stabilisce una connessione al database MySQL
    MYSQL *connetti = connessione();

    // Inizializza le variabili per memorizzare il nome e la data dell'esame
    char nome_esame[255] = {0};
    char data_esame[20] = {0};

    // Legge il nome e la data dell'esame dal client
    read(connfd, nome_esame, sizeof(nome_esame));
    read(connfd, data_esame, sizeof(data_esame));

    // Aggiunge il terminatore di stringa
    nome_esame[sizeof(nome_esame) - 1] = '\0';
    data_esame[sizeof(data_esame) - 1] = '\0';

    // Controlla se le stringhe sono vuote
    if (strlen(nome_esame) == 0 || strlen(data_esame) == 0) {
        const char *errore = "Le stringhe non possono essere vuote";
        write(connfd, errore, strlen(errore));
        return;
    }

    // Inizializza la query SQL
    char SQLquery[700];

    // Costruisce la query SQL per inserire un nuovo esame nel database
    snprintf(SQLquery, sizeof(SQLquery),
             "INSERT INTO appelli (appello_nome,appello_data) VALUES ('%s', (STR_TO_DATE('%s','%%Y-%%m-%%d')))",
             nome_esame, data_esame);

    // Esegue la query SQL
    if (mysql_query(connetti, SQLquery) != 0) {
        // Se l'esecuzione della query fallisce, verifica se è a causa di un vincolo di chiave esterna
        if (strstr(mysql_error(connetti), "foreign key costraint fails")) {
            const char *errore = "non esiste nessun esame che abbia questo nome";
            // Invia un messaggio di errore al client
            write(connfd, errore, strlen(errore));
        } else {
            const char *errore = mysql_error(connetti);
            // Invia il messaggio di errore MySQL al client
            write(connfd, errore, strlen(errore));
        }
    } else {
        const char *inserisci = "l'appello è stato inserito.";
        // Invia un messaggio di successo al client
        write(connfd, inserisci, strlen(inserisci));
    }
    // Chiude la connessione MySQL
    mysql_close(connetti);
}


void aggiungi_prenotazione(int connfd) {
    // Stabilisce una connessione al database MySQL
    MYSQL *connetti = connessione();

    // Inizializza le variabili per memorizzare l'ID dell'appello e la matricola dello studente
    int id, matricola;

    // Legge l'ID dell'appello e la matricola dello studente dal client
    read(connfd, &id, sizeof(id));
    read(connfd, &matricola, sizeof(matricola));

    // Inizializza le stringhe per le query SQL
    char corso_studi[700];
    char piano_studi[700];

    // Costruisce la query SQL per ottenere il corso di studi dell'appello
    snprintf(corso_studi, sizeof(corso_studi),
             "SELECT e.corso_studi FROM appelli app JOIN esami e ON app.appello_nome=e.nome WHERE appello_id=%d", id);

    // Esegue la query SQL
    if (mysql_query(connetti, corso_studi) != 0) {
        fprintf(stderr, "Errore nell'esecuzione della query");
    }

    // Ottiene il risultato della query SQL
    MYSQL_RES *risultato_corso_studi = mysql_store_result(connetti);
    if (risultato_corso_studi == NULL) {
        fprintf(stderr, "Errore nel risultato della query");
    }

    // Controlla se l'appello esiste
    int righe = mysql_num_rows(risultato_corso_studi);

    if (!righe) {
        // Se l'appello non esiste, invia un messaggio di errore al client
        const char *errore = "non esiste nessun appelo che abbia questo id";
        write(connfd, errore, strlen(errore));
    } else {
        // Se l'appello esiste, ottiene il corso di studi dell'appello
        MYSQL_ROW righe_corso_studi = mysql_fetch_row(risultato_corso_studi);
        char corso_studi_nome[255];
        sscanf(righe_corso_studi[0], "%[^\n]", corso_studi_nome);

        // Libera la memoria occupata dal risultato della query SQL
        mysql_free_result(risultato_corso_studi);

        // Costruisce la query SQL per ottenere il piano di studi dello studente
        snprintf(piano_studi, sizeof(piano_studi), "SELECT piano_studi FROM studente WHERE matricola=%d", matricola);

        // Esegue la query SQL
        if (mysql_query(connetti, piano_studi) != 0) {
            fprintf(stderr, "Errore durante l'esecuzione della query");
        }

        // Ottiene il risultato della query SQL
        MYSQL_RES *risultato_piano_studi = mysql_store_result(connetti);
        if (risultato_piano_studi == NULL) {
            fprintf(stderr, "Errore nel risultato della query.");
        }

        // Ottiene il piano di studi dello studente
        MYSQL_ROW righe_piano_studi = mysql_fetch_row(risultato_piano_studi);
        char piano_studi_nome[255];
        sscanf(righe_piano_studi[0], "%[^\n]", piano_studi_nome);

        // Libera la memoria occupata dal risultato della query SQL
        mysql_free_result(risultato_piano_studi);

        // Controlla se l'appello è nel piano di studi dello studente
        if (strcmp(corso_studi_nome, piano_studi_nome) != 0) {
            // Se l'appello non è nel piano di studi dello studente, invia un messaggio di errore al client
            const char *errore = "Non esiste nessun esame che abbia questo nome nel tuo corso di studi.";
            write(connfd, errore, strlen(errore));
        } else {
            // Se l'appello è nel piano di studi dello studente, costruisce la query SQL per aggiungere la prenotazione
            char SQLquery[700];
            snprintf(SQLquery, sizeof(SQLquery),
                     "INSERT INTO prenotazione (id_appello, matricola,data_prenotazione) VALUES (%d,%d,NOW())", id,
                     matricola);

            // Esegue la query SQL
            if (mysql_query(connetti, SQLquery) != 0) {
                // Se l'esecuzione della query fallisce, verifica se è a causa di un duplicato
                if (strstr(mysql_error(connetti), "Duplicate entry")) {
                    // Se è a causa di un duplicato, invia un messaggio di errore al client
                    const char *errore = "già rientra una prenotazione per questo appello";
                    write(connfd, errore, strlen(errore));
                } else {
                    // Se è a causa di un altro errore, invia il messaggio di errore MySQL al client
                    const char *errore = mysql_error(connetti);
                    write(connfd, errore, strlen(errore));
                }
            } else {
                // Se l'esecuzione della query ha successo, invia un messaggio di successo al client
                const char *inserisci = "la tua prenotazione è stata inserita con successo.";
                write(connfd, inserisci, strlen(inserisci));

                // Costruisce la query SQL per ottenere il numero di prenotazioni per l'appello
                char query[255];
                snprintf(query, sizeof(query), "SELECT COUNT (*) FROM prenotazione WHERE id_appello=%d", id);

                // Esegue la query SQL
                if (mysql_query(connetti, query) != 0) {
                    fprintf(stderr, "Errore nella query mysql.");
                }

                // Ottiene il risultato della query SQL
                MYSQL_RES *risultato = mysql_store_result(connetti);

                if (risultato == NULL) {
                    fprintf(stderr, "Errore nella store result");
                }

                // Ottiene il numero di prenotazioni per l'appello
                int count;
                MYSQL_ROW riga = mysql_fetch_row(risultato);
                sscanf(riga[0], "%d", &count);

                // Invia il numero di prenotazioni per l'appello al client
                write(connfd, &count, sizeof(count));

                // Libera la memoria occupata dal risultato della query SQL
                mysql_free_result(risultato);
            }
        }
    }

    // Pulisce il buffer di input
    fflush(stdin);

    // Chiude la connessione MySQL
    mysql_close(connetti);
}




int main() {
    // Inizializza i file descriptor per il socket di ascolto e la connessione, la richiesta del client e l'indirizzo del server
    int listenfd, connfd = -1;
    int request;
    struct sockaddr_in servaddr;

    // Crea un socket TCP e controlla se la creazione ha avuto successo
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore durante la creazione della socket");
        exit(1);
    } else {
        printf("Socket creata con successo\n");
    }

    // Inizializza la struttura servaddr
    servaddr.sin_family = AF_INET;

    // Assegna l'indirizzo IP del server e controlla se l'operazione di inet_pton ha avuto successo
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Inizializza la porta del server
    servaddr.sin_port = htons(5000);

    // Lega il socket all'indirizzo del server e controlla se l'operazione di bind ha avuto successo
    if ((bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        perror("Errore durante l'operazione di bind");
        exit(1);
    } else {
        printf("Bind effettuato con successo\n");
    }

    // Mette il server in ascolto per le connessioni in entrata e controlla se l'operazione di listen ha avuto successo
    if ((listen(listenfd, 2048)) < 0) {
        perror("Errore durante l'operazione di listen");
        exit(1);
    } else {
        printf("Server in ascolto\n");
    }

    // Inizializza il set di file descriptor per la select e imposta max_fd al file descriptor del socket di ascolto
    fd_set readset;
    int max_fd;
    // Imposta max_fd al file descriptor del socket di ascolto
    max_fd = listenfd;

    // Entra in un ciclo infinito per gestire le connessioni in entrata e le richieste dei client
    while (1) {

        // Azzera il set di file descriptor per la select
        FD_ZERO(&readset);

        // Aggiunge il file descriptor del socket di ascolto al set
        FD_SET(listenfd, &readset);

        // Se c'è una connessione attiva, aggiunge anche il file descriptor della connessione al set
        if (connfd > -1) {
            FD_SET(connfd, &readset);
        }

        // Chiama la funzione select per attendere un'attività su uno dei file descriptor nel set
        if (select(max_fd + 1, &readset, NULL, NULL, NULL) < 0) {
            perror("Errore durante la select");
        }

        // Se c'è un'attività sul socket di ascolto, accetta la nuova connessione
        if (FD_ISSET(listenfd, &readset)) {
            if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) < 0) {
                perror("Errore durante l'accept");
            }

            // Aggiorna max_fd se necessario
            if (connfd > max_fd) {
                max_fd = connfd;
            }
        }

        // Se c'è un'attività sulla connessione, legge la richiesta del client
        if (FD_ISSET(connfd, &readset)) {
            if (read(connfd, &request, sizeof(request))) {
                // Se la richiesta è 1, chiama la funzione aggiungi_esame
                if (request == 1) {
                    aggiungi_esame(connfd);
                }
                    // Se la richiesta è 2, chiama la funzione aggiungi_prenotazione
                else if (request == 2) {
                    aggiungi_prenotazione(connfd);
                }
            }
        }
    }
}