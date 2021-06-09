#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define Client_Server_Main  "../etc/client_server_main_fifo"


int main() {


    int fd_client_server_main;
    char Client_Server_ID[BUFSIZ] = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ] = "../tmp/server_client_";


    /* Criar o pipe principal */
    mkfifo(Client_Server_Main, 0666);
    printf("Server ON.\n");

    fd_client_server_main = open(Client_Server_Main, O_RDONLY);
    while (1) {
        /* Abrir o main pipe e ficar à espera que os users se conectem */
        char buf[BUFSIZ];
        ssize_t tamanho_read;
        tamanho_read = read(fd_client_server_main, buf, BUFSIZ);
        if (tamanho_read > 0) {
            int status;
            if (fork() == 0) {
                char path_server_client_id[BUFSIZ];
                strcat(strcpy(path_server_client_id, Server_Client_ID), buf);
                char path_client_server_id[BUFSIZ];
                strcat(strcpy(path_client_server_id, Client_Server_ID), buf);

                if (mkfifo(path_server_client_id, 0666) == -1) printf("Erro creating FIFO\n");
                if (mkfifo(path_client_server_id, 0666) == -1) printf("Erro creating FIFO\n");

                int fd_server_client_id = open(path_server_client_id, O_WRONLY);
                int fd_client_server_id = open(path_client_server_id, O_RDONLY);

                char *con = "Connection Accepted";
                if (write(fd_server_client_id, con, 21) < 0) {
                    perror("Erro na escrita:");
                    //_exit(-1);
                }
                char response[BUFSIZ];
                //memset(response,0,sizeof(response));
                ssize_t response_size;
                response_size = read(fd_client_server_id, response, sizeof(response));
                response[strlen(response)] = '\0';//
                if (response_size > 0) {
                    char *response_array[100];
                    memset(response_array, 0, sizeof(response_array));
                    char *p = strtok(response, " ");
                    int response_c = 0;
                    while (p != NULL) {
                        strcpy(response_array[response_c++], p);
                        p = strtok(NULL, " ");
                    }

                    if (response_c <= 0) {
                        printf("No argument provided\n");
                        _exit(0);
                    } else {
                        if (strcmp(response_array[0], "transform") == 0) {//argv[5][2]
                            if (response_c < 4) {
                                printf("Not enough arguments in program call\n");
                            }
                            char message[] = "Command transform recieved from user ";
                            strcat(message, buf);
                            printf("%s\n", message);
                        } else {
                            if (strcmp(response_array[0], "status") == 0) {
                                if (response_c > 1) {
                                    printf("Too many arguments in program call\n");
                                } else {

                                    char message[] = "Command status recieved from user ";
                                    strcat(message, buf);
                                    printf("%s\n", message);
                                }
                            }
                        }
                    }
                }

                close(fd_server_client_id);
                close(fd_client_server_id);

                unlink(path_server_client_id);
                unlink(path_client_server_id);
                //memset(buf, 0, sizeof(buf));
                _exit(0);
            }


        }
        //break;
    }
    int status;
    wait(&status);
    sleep(3);
    printf("bomdia");


    close(fd_client_server_main);
    unlink(Client_Server_Main);
    return 0;
}