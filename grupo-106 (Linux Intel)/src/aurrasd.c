#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define Client_Server_Main  "../etc/client_server_main_fifo"


int main()
{


    int fd_client_server_main;
    char *Client_Server_ID  = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ]  = "../tmp/server_client_";


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

                if (mkfifo(path_server_client_id, 0666) == -1) printf("Erro creating FIFO");
                if (mkfifo(path_client_server_id, 0666) == -1) printf("Erro creating FIFO");

                int fd_server_client_id = open(path_server_client_id, O_WRONLY);
                int fd_client_server_id = open(path_client_server_id, O_RDONLY);

                char *con = "Connection Accepted";
                if (write(fd_server_client_id, con, 21) < 0) {
                    perror("Erro na escrita:");
                    _exit(-1);
                }
                char response[BUFSIZ];
                ssize_t response_size;
                response_size = read(fd_client_server_id, response, sizeof(response));
                if (response_size > 0) {
                    char *response_array[100];
                    memset(response_array, 0, sizeof(response_array));
                    char *p = strtok(response, " ");
                    int response_c = 0;
                    while (p != NULL) {
                        response_array[response_c++] = p;
                        p = strtok(NULL, " ");
                    }
                    printf("\n%d\n", response_c);
                    if(response_c<=0) {
                        printf("No argument provided");
                    }
                    else {
                        if (strcmp(response_array[0], "transform") == 0) {//argv[5][2]
                            printf("Command transform recieved1\n");
                            if (response_c < 4) {
                                printf("Not enough arguments in program call");
                            }
                            printf("Command transform recieved");
                        }
                        else{
                            if (strcmp(response_array[0], "status") == 0) {
                                if (response_c > 1) {
                                    printf("Too many arguments in program call");
                                }else printf("Command status recieved");
                            }
                        }
                    }
                }



                close(fd_server_client_id);
                close(fd_client_server_id);

                unlink(path_server_client_id);
                unlink(path_client_server_id);
                memset(buf, 0, sizeof(buf));
                _exit(0);
            }
            else {
                wait(&status);
                sleep(4);
                printf("bomdia");
                }

        }
    }
    return 0;
}