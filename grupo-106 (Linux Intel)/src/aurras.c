#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define Client_Server_Main  "../etc/client_server_main_fifo"


int main(int argc, char **argv) {
    char Client_Server_ID[BUFSIZ] = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ] = "../tmp/server_client_";

    int fd_client_server_main;
    char str[140];
    memset(str, 0, sizeof(str));
    char read_id[140];
    memset(read_id, 0, sizeof(read_id));
    snprintf(str, sizeof(str), "%d", (int) getpid());
    puts(str);

    /* Pedir coneccao ao servidor */
    fd_client_server_main = open(Client_Server_Main, O_WRONLY);
    if (fd_client_server_main == -1) printf("error accessing fd_client_server_main");
    if (write(fd_client_server_main, str, sizeof(str)) < 0) {
        perror("Write:");//print error
        exit(-1);
    }
    sleep(1); //FALTA ARRANJAR UMA FORMA DE VER SE O SERVIDOR CONSEGUIU CRIAR O FIFO OU O FORK

    strcat(Server_Client_ID, str);
    int fd_server_client_id = open(Server_Client_ID, O_RDONLY);
    if (fd_server_client_id == -1) {
        printf("Error accessing private Server_Client_FIFO.Server out of memory or Max users connected.\n");
        return -1;
    }

    strcat(Client_Server_ID, str);
    int fd_client_server_id = open(Client_Server_ID, O_WRONLY);
    if (fd_client_server_id == -1) {
        printf("Error accessing private Client_Server_FIFO.Server out of memory or Max users connected.\n");
        return -1;
    }

    if (read(fd_server_client_id, read_id, sizeof(read_id)) < 0) {
        perror("Erro na Leitura:"); //error check
        exit(-1);
    }
    printf("\nServer Response: %s\n", read_id);

    //Checking what the user wants to do

    if (argc <= 1) {
        printf("No argument provided");
        return -1;
    } else {
        if (strcmp(argv[1], "transform") == 0) {//argv[5][2]
            char command[BUFSIZ];
            memset(command, 0, sizeof(command));
            for (int i = 1; argv[i]; i++) {
                strcat(command, argv[i]);
                strcat(command, " ");
            }
            printf("\n\n%s\n\n", command);
            if (argc < 5) {
                printf("Not enough arguments in program call");
                return -1;
            } else {
                if (write(fd_client_server_id, command, sizeof(command)) < 0) {
                    perror("Write Error:"); //error check
                    exit(-1);
                }
            }
        } else if (strcmp(argv[1], "status") == 0) {
            if (argc > 2) {
                printf("Too many arguments in program call");
                return -1;
            } else {
                if (write(fd_client_server_id, argv[1], sizeof(argv[1])) < 0) {
                    perror("Write Error:"); //error check
                    exit(-1);
                }
            }
        }
    }

    close(fd_client_server_main);
    close(fd_server_client_id);
    close(fd_client_server_id);


    /* remove the FIFO */

    return 0;
}