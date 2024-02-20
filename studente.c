#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

// Funzione utilizzata per pulire il buffer di input
void clear_input_buffer() {
    int buffer;
    while ((buffer = getchar()) != '\n' && buffer != EOF);
}

int main(int argc, char **argv) {
    int socketfd, sceltavis, scelta;
    struct sockaddr_in servaddr;
    int matricola;
    char password[255] = {0};
    char risultato[255] = {0};

//Controllo se l'utente ha inserito l'indirizzo IP
    if (argc != 2) {
        fprintf(stderr, "Errore nell'inserimento dell'indirizzo IP. %s è un inserimento errato!", argv[0]);
        exit(1);
    } else {
        printf("Indirizzo IP: %s\n", argv[1]);
    }

    do {

        // Creazione della socket
        if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Errore durante la creazione della socket");
            exit(1);
        } else {
            printf("Socket creata con successo!\n");
        }

        // Inizializzazione della struttura servaddr
        servaddr.sin_family = AF_INET;

        // Conversione dell'indirizzo IP da stringa a formato binario
        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
            fprintf(stderr, "Errore durante l'operazione inet_pton");
            exit(1);
        } else {
            printf("Indirizzo IP convertito con successo!\n");
        }

        // Inizializzazione della porta
        servaddr.sin_port = htons(8081);

        int tentativi = 3;

        // Tentativo di connessione al server
        do {
            if (connect(socketfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
                printf("Ritento la connessione...\n");
                tentativi--;
                sleep(2);
            } else {
                break;
            }

            if (tentativi == 0) {
                sleep(3);

                if (connect(socketfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
                    printf("Non è stato possibile connettersi.");
                    exit(1);
                } else {
                    printf("Connessione stabilita con successo.\n");
                }
            }
        } while (tentativi > 0);


        // Richiesta delle credenziali per il login
        printf("La connessione è stata stabilita con successo. Inserire le credenziali da studente:\n");
        printf("Inserisci la matricola: ");
        scanf("%d", &matricola);
        printf("\n");

        int c;
        while ((c = getchar()) != '\n' && c != EOF);


        printf("Inserisci la password: ");
        fgets(password, sizeof(password), stdin);
        password[strlen(password) - 1] = 0;


        // Invio della matricola al server
        if (write(socketfd, &matricola, sizeof(matricola)) < 0) {
            printf("Non è stato possibile continuare la connessione, riprovo");
            close(socketfd);
            continue;
        }

        // Invio della password al server
        if (write(socketfd, &password, sizeof(password)) < 0) {
            printf("Non è stato possibile continuare la connessione, riprovo");
            close(socketfd);
            continue;
        }

        // Ricezione del risultato del login
        if (read(socketfd, risultato, sizeof(risultato)) < 0) {
            printf("Non è stato possibile continuare la connessione, riprovo.");
            close(socketfd);
            continue;
        }

        // Controllo del risultato del login tramite il confronto con la stringa "Le credenziali non sono corrette"
        if (strcmp(risultato, "Le credenziali non sono corrette") == 0) {
            printf("Non è stato possibile effettuare il login perchè: %s", risultato);
            exit(1);
        } else {
            printf("E' stato possibile effetuuare il login perchè: %s\n", risultato);
        }

        // Menu per la scelta delle operazioni
        while (1) {
            printf("Seleziona una delle opzioni da effettuare : \n");
            printf("1) Visualizza gli appelli che sono disponibili; \n");
            printf("2) Prenotati ad un appello; \n");
            printf("3) Effettua il logout; \n");
            scanf("%d", &scelta);
            printf("\n");


            // Invio della scelta al server
            if (write(socketfd, &scelta, sizeof(scelta)) < 0) {
                printf("Disconeessione, riprovo.\n");
                close(socketfd);
                continue;
            }

            // Controllo della scelta effettuata
            if (scelta == 1) {
                int id;
                char nome_esame[500] = {0};
                char data_esame[20] = {0};
                int num_righe;

                printf("\n Scegli un opzione per poter visualizzare gli appelli: \n ");
                printf("1) Visualizza tutti gli appelli disponibili del tuo corso di studi; \n ");
                printf("2) Visualizza gli appelli di un solo esame che riguardano il tuo corso di studi; \n ");
                printf("Inserisci la tua scelta riguardante gli appelli: ");
                scanf("%d ", &sceltavis);

                clear_input_buffer();


                // Invio della scelta al server
                if (write(socketfd, &sceltavis, sizeof(sceltavis)) < 0) {
                    printf("Disconessione, riprovo.\n");
                    close(socketfd);
                    continue;
                }

                printf("Scelta con successo\n");

                // Controllo della scelta effettuata
                if (sceltavis == 2) {
                    char esami[500] = {0};
                    printf("\nInserisci il nome dell'esame di cui cerchi gli appelli: ");
                    fgets(esami, sizeof(esami), stdin);
                    esami[strlen(esami) - 1] = '\0';

                    // Invio del nome dell'esame al server
                    if (write(socketfd, esami, sizeof(esami)) < 0) {
                        printf("Disconnessione, riprovo.\n");
                        close(socketfd);
                        continue;
                    }
                }


                // Ricezione degli appelli disponibili
                if (read(socketfd, &num_righe, sizeof(num_righe)) < 0) {
                    printf("Disconnessione, riprovo.\n");
                    close(socketfd);
                    continue;
                }

                // Controllo del numero di appelli disponibili
                if (num_righe == 0) {
                    printf("\nNon ci sono appelli disponibili\n");
                } else {
                    printf("Gli appelli disponibili sono i seguenti: \n");
                    for (int i = 0; i < num_righe; i++) {

                        // Ricezione dell'id dell'appello
                        if (read(socketfd, &id, sizeof(id)) < 0) {
                            printf("Disconnessione, riprovo.\n");
                            close(socketfd);
                            continue;
                        }

                        // Ricezione del nome dell'esame
                        if (read(socketfd, nome_esame, sizeof(nome_esame)) < 0) {
                            printf("Disconnessione, riprovo.\n");
                            close(socketfd);
                            continue;
                        }
                        nome_esame[sizeof(nome_esame) - 1] = '\0'; // Null-terminate the string

                        // Ricezione della data dell'appello
                        if (read(socketfd, data_esame, sizeof(data_esame)) < 0) {
                            printf("Disconnessione, riprovo.\n");
                            close(socketfd);
                            continue;
                        }
                        data_esame[sizeof(data_esame) - 1] = '\0'; // Null-terminate the string

                        printf("ID: %d \t Nome esame: %s  \t Data esame:%s  \n", id, nome_esame, data_esame);
                    }
                }
            } else if (scelta == 2) {
                int cod_app;

                printf("Digitare il codice dell'appello a cui vuoi prenotarti: \n");
                scanf("%d", &cod_app);

                // Invio del codice dell'appello al server
                if (write(socketfd, &cod_app, sizeof(cod_app)) < 0) {
                    printf("Disconnessione, riprovo.\n");
                    close(socketfd);
                    continue;
                }


                // Ricezione del risultato della prenotazione
                if (write(socketfd, &matricola, sizeof(matricola)) < 0) {
                    printf("Disconnessione, riprovo.\n");
                    close(socketfd);
                    continue;
                }

                char risultato[500] = {0};


                // Ricezione del risultato della prenotazione
                if (read(socketfd, risultato, sizeof(risultato)) < 0) {
                    printf("Disconnessione, riprovo.\n");
                    close(socketfd);
                    continue;
                }


                // Controllo del risultato della prenotazione
                if (strcmp(risultato, "La prenotazione è stata effettuata con successo.") == 0) {
                    int num;
                    if (read(socketfd, &num, sizeof(num)) < 0) {
                        printf("Disconnessione, riprovo.\n");
                        close(socketfd);
                        continue;
                    }
                    printf("Numero della prenotazione: %d \n", num);
                } else {
                    printf("Risultato: %s\n", risultato);
                }
            } else if (scelta == 3) {
                printf("Il logout è stato effettuato con successo.\n");
                close(socketfd);
                exit(1);
            }
        }
    } while (1);
}

