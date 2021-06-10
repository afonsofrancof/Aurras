#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define Client_Server_Main "../etc/client_server_main_fifo"

int main() {

    int fd_client_server_main;
    char Client_Server_ID[BUFSIZ] = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ] = "../tmp/server_client_";

    /* Criar o pipe principal */
    mkfifo(Client_Server_Main, 0666);
    printf("Server ON.\n");

    fd_client_server_main = open(Client_Server_Main, O_RDONLY);
    const int max_alto = 3;
    const int max_baixo = 3;
    int *filtros_uso = mmap(NULL, 4 * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, 0, 0);
    for (int i = 0; i < 4; i++)
        filtros_uso[i] = 0;
    while (1) {
        /* Abrir o main pipe e ficar à espera que os users se conectem */
        char buf[BUFSIZ];
        ssize_t tamanho_read;
        tamanho_read = read(fd_client_server_main, buf, BUFSIZ);
        if (tamanho_read > 0) {
            pid_t pid;
            if ((pid = fork()) == 0) {
                char path_server_client_id[BUFSIZ];
                strcat(strcpy(path_server_client_id, Server_Client_ID), buf);
                char path_client_server_id[BUFSIZ];
                strcat(strcpy(path_client_server_id, Client_Server_ID), buf);

                if (mkfifo(path_server_client_id, 0666) == -1)
                    printf("Erro creating FIFO\n");
                if (mkfifo(path_client_server_id, 0666) == -1)
                    printf("Erro creating FIFO\n");

                int fd_server_client_id = open(path_server_client_id, O_WRONLY);
                int fd_client_server_id = open(path_client_server_id, O_RDONLY);

                char *con = "Connection Accepted";
                if (write(fd_server_client_id, con, 20) < 0) {
                    perror("Erro na escrita:");
                    //_exit(-1);
                }
                char response[BUFSIZ];
                //memset(response,0,sizeof(response));
                ssize_t response_size;
                response_size = read(fd_client_server_id, response, sizeof(response));
                response[strlen(response)] = '\0'; //
                if (response_size > 0) {
                    char response_array[10][30];
                    memset(response_array, 0, sizeof(response_array));
                    char *p = strtok(response, " ");
                    int response_c = 0;
                    while (p != NULL) {
                        //FALTA LER DO FICHEIRO DE CONFIG
                        /*if (strcmp(p, "alto") == 0) { p = "aurrasd-gain-double"; }
                        else if (strcmp(p, "baixo") == 0) { p = "aurrasd-gain-half"; }
                        else if (strcmp(p, "echo") == 0) { p = "aurrasd-echo"; }
                        else if (strcmp(p, "rapido") == 0) { p = "aurrasd-tempo-double"; }
                        else if (strcmp(p, "lento") == 0) { p = "aurrasd-tempo-half"; }*/
                        strcpy(response_array[response_c++], p);
                        p = strtok(NULL, " ");
                    }

                    if (response_c <= 0) {
                        printf("No argument provided\n");
                        _exit(0);
                    } else {
                        if (strcmp(response_array[0], "transform") == 0) { //argv[5][2]
                            if (response_c < 4) {
                                printf("Not enough arguments in program call\n");
                            } else {
                                char message[] = "Command transform recieved from user ";
                                strcat(message, buf);
                                printf("%s\n", message);
                                int consegue = 1; // FALTA VER SE ESTA DISPONIVEL
                                const int n_comandos = response_c - 3;
                                if (consegue) {
                                    char path[] = "../bin/aurrasd-filters/";
                                    int pd[n_comandos - 1][2];
                                    if (n_comandos == 1) {
                                        if (fork() == 0) {

                                            int fd_r = open(response_array[1], O_RDONLY);
                                            int fd_w = open(response_array[2], O_CREAT | O_WRONLY, 0666);
                                            dup2(fd_r, 0);
                                            close(fd_r);
                                            dup2(fd_w, 1);
                                            close(fd_w);
                                            strcat(path, response_array[3]);
                                            execl(path, path, NULL);
                                        }
                                    } else {
                                        if (pipe(pd[0]) == -1) {
                                            printf("Pipe Error\n");
                                            _exit(-1);
                                        }
                                        for (int i = 0; i < n_comandos; i++) {

                                            if (i == 0) {
                                                printf("%s\n", response_array[3]);
                                                if (fork() == 0) {
                                                    int fd = open(response_array[1], O_RDONLY);
                                                    dup2(fd, 0);
                                                    close(fd);
                                                    close(pd[i][0]);
                                                    dup2(pd[i][1], 1);
                                                    close(pd[i][1]);
                                                    //FALTA IR BUSCAR AO ARGV O FILTER PATH
                                                    strcat(path, response_array[3]);
                                                    execl(path, path, NULL);
                                                }
                                            } else if (i == n_comandos - 1) {
                                                printf("%s\n", response_array[response_c - 1]);
                                                if (fork() == 0) {
                                                    int fd = open(response_array[2], O_CREAT | O_WRONLY, 0666);
                                                    dup2(fd, 1);
                                                    close(fd);
                                                    dup2(pd[i - 1][0], 0);
                                                    close(pd[i - 1][1]);
                                                    close(pd[i - 1][0]);
                                                    strcat(path, response_array[response_c - 1]);
                                                    execl(path, path, NULL);
                                                }

                                            } else {
                                                if (pipe(pd[i]) == -1) {
                                                    printf("Pipe Error\n");
                                                    _exit(-1);
                                                }
                                                if (fork() == 0) {
                                                    dup2(pd[i - 1][0], 0);
                                                    close(pd[i - 1][0]);
                                                    dup2(pd[i][1], 1);
                                                    close(pd[i][1]);
                                                    close(pd[i - 1][1]);
                                                    close(pd[i][0]);
                                                    strcat(path, response_array[i + 3]);
                                                    execl(path, path, NULL);

                                                }
                                            }

                                        }
                                    }
                                    for (int i = 0; i < n_comandos; i++) {
                                        int status;
                                        wait(&status);
                                    }
                                }
                            }
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