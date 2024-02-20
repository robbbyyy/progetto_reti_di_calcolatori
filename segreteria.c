#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <mysql/mysql.h>


// Macro per calcolare il massimo tra due numeri
#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
    x_ > y_ ? x_ : y_;})

// Struttura per gestire le informazioni dello studente
typedef struct {
    int stud_connfd;
    int stud_matricola;
} studente;

void clear_input_buffer() {
    int buffer;
    while ((buffer = getchar()) != '\n' && buffer != EOF);
}


int main(int argc, char **argv) {
    int sockfd, listenfd;
    struct sockaddr_in studaddr, servaddr;
    int indice = 0;
    MYSQL *connetti;
    studente studente_socket[5000];
    char pass[500] = {0};
    char mat_pass[500];
    char esame[500] = {0};
    int scelta_studente, scelta_segreteria;
    char nome[500] = {0};
    char esito_ric[500] = {0};
    char data_appello[50] = {0};

    /*CLIENT*/

    // Controllo del numero di argomenti passati al programma
    if (argc != 2) {
        fprintf(stderr, "Indirizzo IP: %s errato\n", argv[0]);
        exit(1);
    } else {
        printf("Indirizzo IP: %s \n", argv[1]);
    }

    // Creazione della socket del client
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore durante la creazione della socket");
        exit(1);
    } else {
        printf("Socket creata con successo\n");
    }

    // Impostazione dei parametri della socket del client
    studaddr.sin_family = AF_INET;

    // Conversione dell'indirizzo IP da stringa a formato binario
    if (inet_pton(AF_INET, argv[1], &studaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore sulla inet_pton per %s \n", argv[1]);
        exit(1);
    } else {
        printf("Indirizzo IP convertito con successo\n");
    }


    // Impostazione della porta del client
    studaddr.sin_port = htons(5000);

    // Connessione del client al server
    if (connect(sockfd, (struct sockaddr *) &studaddr, sizeof(studaddr)) < 0) {
        perror("Non è stato possibile effettuare la connect");
        exit(1);
    } else {
        printf("Connessione stabilita con successo\n");
    }
    /*SERVER*/

    // Creazione della socket del server
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore durante la creazione della socket");
        exit(1);
    } else {
        printf("Socket creata con successo\n");
    }

    // Impostazione dei parametri della socket del server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8081);

    // Associazione della socket del server a un indirizzo locale
    if ((bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        perror("Non è stato possibile effettuare il bind");
        exit(1);
    } else {
        printf("Bind effettuato con successo\n");
    }

    // Impostazione della socket del server in modalità di ascolto
    if ((listen(listenfd, 5)) < 0) {
        perror("Non è stato possibile effettuare la listen");
        exit(1);
    } else {
        printf("Listen effettuato con successo\n");
    }

    // Inizializzazione della connessione al database
    connetti = mysql_init(NULL);

    // Controllo dell'inizializzazione della connessione al database
    if (connetti == NULL) {
        fprintf(stderr, "inizializzazione mysql errata\n");
        exit(1);
    }

    // Connessione al database
    if (mysql_real_connect(connetti, "127.0.0.1", "root", "ROOT123root__", "universita", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "Errore nella connessione al DataBase universita: %s\n", mysql_error(connetti));
        mysql_close(connetti);
        exit(1);
    } else {
        printf("Connessione al database effettuata con successo\n\n\n");
    }

    // Inizializzazione dei set di descrittori di file per select
    fd_set read_fd_set, write_fd_set, master_fd_set;
    int max_fd;

    FD_ZERO(&master_fd_set);

    FD_SET(sockfd, &master_fd_set);
    max_fd = sockfd;

    FD_SET(listenfd, &master_fd_set);
    max_fd = max(max_fd, listenfd);

    // Ciclo principale del server
    while (1) {
        read_fd_set = master_fd_set;
        write_fd_set = master_fd_set;

        // Ciclo di gestione delle connessioni entranti
        while (1) {
            // Chiamata a select per monitorare i descrittori di file
            if (select(max_fd + 1, &read_fd_set, &write_fd_set, NULL, NULL) < 0) {
                perror("Errore durante l'operazione di select");
            }

            // Gestione delle connessioni entranti
            if (FD_ISSET(listenfd, &read_fd_set)) {
                // Accettazione di una nuova connessione
                if ((studente_socket[indice].stud_connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) < 0) {
                    perror("Duarnte l'accept si è verificato un errore");
                } else {
                    // Aggiunta del nuovo descrittore di file al set
                    FD_SET(studente_socket[indice].stud_connfd, &master_fd_set);
                    max_fd = max(max_fd, studente_socket[indice].stud_connfd);

                    // Lettura delle informazioni dello studente
                    read(studente_socket[indice].stud_connfd, &studente_socket[indice].stud_matricola,
                         sizeof(studente_socket[indice].stud_matricola));
                    read(studente_socket[indice].stud_connfd, pass, sizeof(pass));

                    // Creazione della query SQL per verificare le credenziali dello studente
                    char SQLquery[700];

                    snprintf(SQLquery, sizeof(SQLquery), "SELECT * FROM studente WHERE matricola = %d AND pass = '%s' ",
                             studente_socket[indice].stud_matricola, pass);

                    // Esecuzione della query SQL
                    if (mysql_query(connetti, SQLquery) != 0) {
                        fprintf(stderr, "La query non è andata a buon fine");
                    }

                    // Recupero del risultato della query
                    MYSQL_RES *accesso = mysql_store_result(connetti);
                    if (accesso == NULL) {
                        fprintf(stderr, "Errore nel risultato");
                    }

                    // Controllo del numero di righe restituite dalla query
                    int righe = mysql_num_rows(accesso);
                    mysql_free_result(accesso);

                    // Controllo delle credenziali dello studente
                    if (righe != 1) {
                        strcpy(mat_pass, "Le credenziali non sono corrette");
                        write(studente_socket[indice].stud_connfd, mat_pass, strlen(mat_pass));
                    } else {
                        strcpy(mat_pass, "Le credenziali  sono corrette");
                        write(studente_socket[indice].stud_connfd, mat_pass, strlen(mat_pass));

                        // Incremento del numero di studenti connessi
                        indice++;
                    }
                }
            } else {
                break;
            }
        }

        // Ciclo di gestione delle richieste degli studenti
        for (int i = 0; i < indice; i++) {

            // Controllo se c'è una richiesta da parte dello studente
            if (FD_ISSET(studente_socket[i].stud_connfd, &read_fd_set) && studente_socket[i].stud_connfd != -1) {

                // Lettura della scelta dello studente
                read(studente_socket[i].stud_connfd, &scelta_studente, sizeof(scelta_studente));

                // Gestione delle diverse scelte dello studente
                if (scelta_studente == 1) {
                    // Gestione della richiesta di visualizzazione degli appelli
                    int richiesta;

                    read(studente_socket[i].stud_connfd, &richiesta, sizeof(richiesta));

                    char sqlquery[800];

                    if (richiesta == 1) {
                        // Creazione della query SQL per ottenere gli appelli disponibili per lo studente
                        snprintf(sqlquery, sizeof(sqlquery),
                                 "SELECT app.appello_id, app.appello_nome, DATE_FORMAT(app.appello_data, '%%Y-%%m-%%d') FROM appelli app JOIN esami e ON app.appello_nome = e.nome WHERE e.corso_studi = (SELECT piano_studi FROM studente WHERE matricola=%d)",
                                 studente_socket[i].stud_matricola);

                        // Esecuzione della query SQL
                        if (mysql_query(connetti, sqlquery) != 0) {
                            fprintf(stderr, "Errore durante l'esecuzione della query\n");
                        }
                    } else if (richiesta == 2) {
                        // Gestione della richiesta di visualizzazione degli appelli per un esame specifico
                        read(studente_socket[i].stud_connfd, esame, sizeof(esame));
                        snprintf(sqlquery, sizeof(sqlquery),
                                 "SELECT app.appello_id, app.appello_nome, DATE_FORMAT(app.appello_data, '%%Y-%%m-%%d') FROM appelli app JOIN esami e ON app.appello_nome = e.nome WHERE e.corso_studi = (SELECT piano_studi FROM studente WHERE matricola= %d ) AND e.nome='%s'",
                                 studente_socket[i].stud_matricola, esame);

                        // Esecuzione della query SQL
                        if (mysql_query(connetti, sqlquery) != 0) {
                            fprintf(stderr, "Errore durante l'esecuzione della query");
                        }
                    }

                    // Recupero del risultato della query
                    MYSQL_RES *verifica_app = mysql_store_result(connetti);
                    if (verifica_app == NULL) {
                        fprintf(stderr, "Risultante della mysql fallita");
                    } else {
                        // Invio del numero di righe restituite dalla query allo studente
                        int righe = mysql_num_rows(verifica_app);
                        write(studente_socket[i].stud_connfd, &righe, sizeof(righe));

                        int id;
                        char nome_esame[255];
                        char data_esame[20];

                        MYSQL_ROW riga;

                        // Invio delle informazioni sugli appelli allo studente
                        while ((riga = mysql_fetch_row(verifica_app))) {
                            sscanf(riga[0], "%d", &id);
                            sscanf(riga[1], "%[^\n]", nome_esame);
                            sscanf(riga[2], "%s", data_esame);
                            write(studente_socket[i].stud_connfd, &id, sizeof(id));
                            write(studente_socket[i].stud_connfd, nome_esame, sizeof(nome_esame));
                            write(studente_socket[i].stud_connfd, data_esame, sizeof(data_esame));
                        }
                    }

                    // Liberazione del risultato della query
                    mysql_free_result(verifica_app);
                } else if (scelta_studente == 2) {
                    // Gestione della richiesta di prenotazione di un appello
                    int codice_appello, matricola;
                    char risultato[255] = {0};

                    // Invio della scelta allo studente
                    write(sockfd, &scelta_studente, sizeof(scelta_studente));

                    // Lettura del codice dell'appello e della matricola dello studente
                    read(studente_socket[i].stud_connfd, &codice_appello, sizeof(codice_appello));
                    read(studente_socket[i].stud_connfd, &matricola, sizeof(matricola));

                    // Invio del codice dell'appello e della matricola allo studente
                    write(sockfd, &codice_appello, sizeof(codice_appello));
                    write(sockfd, &matricola, sizeof(matricola));

                    // Lettura del risultato della prenotazione
                    read(sockfd, risultato, sizeof(risultato));

                    // Invio del risultato della prenotazione allo studente
                    write(studente_socket[i].stud_connfd, risultato, sizeof(risultato));

                    // Controllo del risultato della prenotazione
                    if (strcmp(risultato, "La prenotazione è stata effettuata con successo.") == 0) {
                        int count;
                        read(sockfd, &count, sizeof(count));
                    }
                } else if (scelta_studente == 3) {
                    // Gestione della richiesta di disconnessione dello studente
                    close(studente_socket[i].stud_connfd);
                    // Rimozione del descrittore di file dal set
                    FD_CLR(studente_socket[i].stud_connfd, &master_fd_set);
                    // Reset del descrittore di file
                    studente_socket[i].stud_connfd = -1;
                    // Reset della matricola dello studente
                    studente_socket[i].stud_matricola = 0;
                }
            }
        }

        // Controllo se c'è una richiesta da parte della segreteria
        if (FD_ISSET(sockfd, &write_fd_set)) {

            // Stampa del menu delle opzioni disponibili per la segreteria
            printf("Quali delle seguenti opzioni vuoi fare: \n");
            printf("1: Gestire le richieste effettuate dagli studenti.\n");
            printf("2: Inserimento di un nuovo appello.\n");
            printf("Inserire il numero relativo alla scelta: ");
            scanf("%d", &scelta_segreteria);
            printf("\n");

            // Pulizia del buffer di input
            clear_input_buffer();

            // Gestione delle diverse scelte della segreteria
            if (scelta_segreteria == 2) {
                // Gestione della richiesta di inserimento di un nuovo appello
                int richiesta = 1;

                // Invio della richiesta al server
                write(sockfd, &richiesta, sizeof(richiesta));

                // Lettura del nome dell'esame per il quale si vuole inserire l'appello
                printf("Digitare il nome dell'esame di cui si vuole inserire l'appello: ");
                fgets(nome, sizeof(nome), stdin);
                nome[strlen(nome) - 1] = 0;
                printf("\n");

                // Invio del nome dell'esame al server
                write(sockfd, nome, strlen(nome));

                // Lettura della data dell'appello
                printf("Inserire la data in cui si vuole svolgere l'appello (FORMATO: YYYY-MM-GG) : ");
                printf("\n");
                fgets(data_appello, sizeof(data_appello), stdin);
                data_appello[strlen(data_appello) - 1] = 0;

                // Invio della data dell'appello al server
                write(sockfd, data_appello, strlen(data_appello));

                // Lettura dell'esito della richiesta
                read(sockfd, esito_ric, sizeof(esito_ric));
                printf("L'esito della richiesta: %s\n", esito_ric);
            }
        }
    }
}