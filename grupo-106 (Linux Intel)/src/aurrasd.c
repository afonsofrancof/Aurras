#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#define tamanho_filtros_array 10
#define Client_Server_Main "../etc/client_server_main_fifo"

//Variaveis globais a serem alteradas pela main e pelo SIGHandler.
char *array_conf[30];
int max_filters[10] = {0};
int filtros_em_uso[10] = {0};
int n_filtros;
typedef struct espera {
    pid_t pid;
    char comando[150];
    int filtros[tamanho_filtros_array];
    struct espera *prox;
} *ESPERA;
ESPERA main_queue;

void print_espera() {
    while (main_queue) {
        printf("%d - %s\n", main_queue->pid, main_queue->comando);
        main_queue = main_queue->prox;
    }
    for (int i = 0, u = 0; i < n_filtros; i++, u = u + 3) {
        printf("filtro %s: %d/%d (running/max)\n", array_conf[u], filtros_em_uso[i], max_filters[i]);
    }
}

int parse_string_to_string_array(char *string, char **str_array, char *delimit) {
    //Dar parse a uma string num array de char pointers.
    int n_words = 0;
    char *p = strtok(string, delimit);
    while (p != NULL) {
        str_array[n_words++] = p;
        p = strtok(NULL, delimit);
    }
    return n_words;
}

void sigHandler(int signum) {
    printf("Process finished, looking for processes to unpause\n");
    while (main_queue) {
        int filtro_cheio = 0;
        for (int i = 0; i < n_filtros; i++) {
            if (filtros_em_uso[i] + main_queue->filtros[i] > max_filters[i]) filtro_cheio = 1;
        }
        if (filtro_cheio == 0) {
            for (int i = 0; i < n_filtros; i++) {
                filtros_em_uso[i] += main_queue->filtros[i];
            }
            kill(main_queue->pid, SIGALRM);
        }
        main_queue = main_queue->prox;
    }
    /* for (int i = 0; i < n_filtros; i++) {
         filtros_em_uso[i] += main_queue
     }*/
}


int main(int argc, char **argv) {

    signal(SIGUSR1, sigHandler);
    switch (argc) {
        case 1:
            printf("No arguments provided\n");
            return -1;
        case 2:
            printf("Too few arguments in program call\n");
            return -1;
        case 3:
            break;
        default:
            printf("Too many arguments in program call\n");
            return -1;
    }
    //Abrir ficheiro de config e dar parse para um array de strings;
    char conf_buf[BUFSIZ];
    int conf_fd = open(argv[1], O_RDONLY);
    //Ler esse tal ficheiro
    read(conf_fd, conf_buf, sizeof(conf_buf));
    close(conf_fd);
    //Dar parse do ficheiro de config para um array de char pointers.
    int size_array_conf = parse_string_to_string_array(conf_buf, array_conf, " \n");
    //Definir o numero de filtros (variavel global *Importante*) para uso varias vezes ao longo do programa.
    n_filtros = size_array_conf / 3;
    for (int i = 2, u = 0; u < n_filtros; i = i + 3, u++) {
        max_filters[u] = (int) strtol(array_conf[i], NULL, 10);
    }
    for (int i = 0; i < n_filtros; i++) {
        printf("%d\n", max_filters[i]);
    }
    //printf("%u\n", size_array_conf);
    /*for (int i = 0; i < size_array_conf; i = i + 3) {
        printf("%i - %s %s %d\n", i, array_conf[i], array_conf[i + 1], (int) strtol(array_conf[i + 2], NULL, 10));
    }*/


    // Parte 1 - Diretoria temporaria para os pipes com nome (FIFO) para comunicacao privada entre cliente e servidor.
    char Client_Server_ID[BUFSIZ] = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ] = "../tmp/server_client_";

    // Criar o pipe principal.
    mkfifo(Client_Server_Main, 0666);
    printf("Server ON.\n");

    //Abrir o main pipe.
    int fd_client_server_main;
    fd_client_server_main = open(Client_Server_Main, O_RDONLY);

    main_queue = malloc(sizeof(struct espera));
    main_queue = NULL;

    int pipe_filho_main[2];
    if (pipe(pipe_filho_main) == -1) printf("Erro ao criar pipe");
    //Este loop serve para podermos ter mais que 1 processo em execucao simultanea.
    while (1) {

        // Abrir o main pipe e ficar à espera que os users se conectem (escrevam o ser pid).
        char buf[BUFSIZ];
        ssize_t tamanho_read;
        tamanho_read = read(fd_client_server_main, buf, sizeof(buf));
        //Se o que lermos do main pipe for maior que 0, ou seja, alguem se conectou e escreveu o seu pid , continuamos.
        if (tamanho_read > 0) {
            char *main_response[30] = {0};
            int main_response_size = parse_string_to_string_array(buf, main_response, ";");

            if (main_response_size > 1) {
                if (strcmp(main_response[0], "remover") == 0) {
                    int main_n_filtros = (int) strtol(main_response[1], NULL, 10);
                    for (int i = 0; i < main_n_filtros; i++) {
                        filtros_em_uso[i] -= (int) strtol(main_response[i + 2], NULL, 10);
                    }
                } else if (strcmp(main_response[0], "adicionar") == 0) {
                    int main_n_filtros = (int) strtol(main_response[1], NULL, 10);
                    for (int i = 0; i < main_n_filtros; i++) {
                        filtros_em_uso[i] += (int) strtol(main_response[i + 2], NULL, 10);
                    }
                } else if (strcmp(buf, "status") != 0) {
                    main_queue->pid = (pid_t) strtol(main_response[0], NULL, 10);
                    strcpy(main_queue->comando, main_response[1]);
                    char *fields_pipe_filtros[tamanho_filtros_array] = {0};
                    parse_string_to_string_array(main_response[2], fields_pipe_filtros, ",");
                    for (int i = 0; i < n_filtros; i++) {
                        main_queue->filtros[i] = (int) strtol(fields_pipe_filtros[i], NULL, 10);
                    }
                    main_queue->prox = malloc(sizeof(struct espera));
                    main_queue->prox = NULL;

                }

            } else {

                pid_t pid;
                //Nesta parte criamos o main Filho. Este filho ira representar um cliente.
                if ((pid = fork()) == 0) {

                    /*
                     * Concatenamos o que lemos da main pipe (o pid do processo que se conectou ao server) com a path da
                       diretoria onde sera guardado o private pipe do cliente com o server.
                     */
                    strcat(Server_Client_ID, buf);
                    strcat(Client_Server_ID, buf);

                    //Criar os fifos nessa diretoria.
                    if (mkfifo(Server_Client_ID, 0666) == -1)
                        printf("Erro creating FIFO\n");
                    if (mkfifo(Client_Server_ID, 0666) == -1)
                        printf("Erro creating FIFO\n");

                    //Abrir os fifos para comunicacoa
                    int fd_server_client_id = open(Server_Client_ID, O_WRONLY);
                    int fd_client_server_id = open(Client_Server_ID, O_RDONLY);

                    //Reportar de volta para o cliente que a coneccao sucedeu.
                    char *con = "Connection Accepted";
                    if (write(fd_server_client_id, con, 20) < 0) {
                        perror("Erro na escrita:");
                        //_exit(-1);
                    }

                    //Agora que a coneccao foi sucedida, o server vai ler o request do cliente na private pipe.
                    char response[BUFSIZ] = {0};
                    ssize_t response_size;
                    response_size = read(fd_client_server_id, response, sizeof(response));
                    printf("%s\n", response);
                    //Verificamos denovo se foi lida alguma coisa do private pipe , e se sim, avancamos.
                    if (response_size > 0) {

                        ////Dar parse ao comando do user para um array de char pointers.
                        char *response_array[30] = {0};
                        char response_copy_for_pipe[sizeof(response)];
                        strcpy(response_copy_for_pipe, response);
                        int response_c = parse_string_to_string_array(response, response_array, " \0");

                        //Verificar se o comando é o status e executalo se assim o for
                        if (strcmp(response_array[0], "status") == 0) {
                            if (response_c > 1) {
                                printf("Too many arguments in program call\n");
                            } else {
                                print_espera();
                                char status_buffer[] = "status";
                                write(fd_client_server_main, status_buffer, sizeof(status_buffer));
                                _exit(0);
                            }
                        }

                        /*
                         * Substituir os filtros dados pelo user pelo nome do ficheiro correspondente
                         * tendo em conta o ficheiro de config e adicionar os filtros que o user quer
                         * usar num novo array local.
                        */
                        int filtros_cliente[10] = {0};
                        for (int i = 3; i < response_c; i++) {
                            for (int u = 0, l = 0; u < size_array_conf; u = u + 3, l++) {
                                if (strcmp(response_array[i], array_conf[u]) == 0) {
                                    response_array[i] = array_conf[u + 1];
                                    filtros_cliente[u / 3]++;
                                    break;
                                }
                            }
                        }

                        /*for (int i = 0; i < n_filtros; i++) {
                            printf("CLIENTE - %d | MAX - %d\n", filtros_cliente[i], max_filters[i]);
                        }*/
                        //Dar match no comando do user a um dos comandos disponiveis
                        if (response_c <= 0) {
                            printf("No argument provided\n");
                            _exit(0);
                        } else {
                            if (strcmp(response_array[0], "transform") == 0) {
                                if (response_c < 4) {
                                    printf("Not enough arguments in program call\n");
                                } else {
                                    const int n_comandos = response_c - 3;
                                    printf("comando transform\n");
                                    //Adicionar o filho que vai ficar a espera na queue
                                    int continua = 1;
                                    if (main_queue != NULL) { continua = 0; }
                                    for (int i = 0; i < n_filtros; i++) {
                                        if (filtros_em_uso[i] + filtros_cliente[i] > max_filters[i]) continua = 0;
                                    }
                                    if (continua == 0) {
                                        char send_parent_string[100] = {0};
                                        snprintf(send_parent_string, sizeof(send_parent_string), "%d", (int) getpid());
                                        strcat(send_parent_string, ";");
                                        strcat(send_parent_string, response_copy_for_pipe);
                                        strcat(send_parent_string, ";");
                                        for (int i = 0; i < n_filtros; i++) {
                                            char temp[4];
                                            snprintf(temp, sizeof(temp), "%d", (int) filtros_cliente[i]);
                                            strcat(send_parent_string, temp);
                                            strcat(send_parent_string, ",");
                                        }
                                        write(fd_client_server_main, send_parent_string, sizeof(send_parent_string));
                                        pause();
                                    }
                                    char adicionar[] = "adicionar;";
                                    char tamanho[3] = {0};
                                    sprintf(tamanho, "%d;", n_filtros);
                                    strcat(adicionar, tamanho);
                                    for (int i = 0; i < n_filtros; i++) {
                                        char tmp[3] = {0};
                                        snprintf(tmp, sizeof(tmp), "%d;", filtros_cliente[i]);
                                        strcat(adicionar, tmp);
                                    }
                                    printf("ADD = %s\n", adicionar);

                                    write(fd_client_server_main, adicionar, sizeof(adicionar));
                                    /*
                                     * Quando para de ficar a espera dizer ao sigHandler2 para eleminar o primeiro da
                                     * lista que de certeza que é este filho porque quando esta cheio so 1 pode executar
                                     * de cada vez e se este foi unpaused entao este é o proximo da lista.(Se 3 forem
                                     * unpaused ao mesmo tempo , vao ser 3 removidos da queue também.
                                    */

                                    char path[sizeof(argv[2])];
                                    strcat(path, argv[2]);
                                    strcat(path, "/"); //MUDAR
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
                                            execl(path, response_array[3], NULL);
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
                                                    close(pd[i][0]);
                                                    int fd = open(response_array[1], O_RDONLY);
                                                    dup2(fd, 0);
                                                    close(fd);
                                                    dup2(pd[i][1], 1);
                                                    close(pd[i][1]);
                                                    //FALTA IR BUSCAR AO ARGV O FILTER PATH
                                                    strcat(path, response_array[3]);
                                                    execlp(path, path, NULL);
                                                    _exit(0);
                                                }
                                            } else if (i == n_comandos - 1) {
                                                printf("%s\n", response_array[response_c - 1]);
                                                if (fork() == 0) {
                                                    close(pd[i - 1][1]);
                                                    int fd = open(response_array[2], O_CREAT | O_WRONLY, 0666);
                                                    dup2(fd, 1);
                                                    close(fd);
                                                    dup2(pd[i - 1][0], 0);
                                                    close(pd[i - 1][0]);
                                                    strcat(path, response_array[response_c - 1]);
                                                    execlp(path, path, NULL);
                                                    _exit(0);
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
                                                    execlp(path, path, NULL);
                                                    _exit(0);
                                                }
                                            }

                                        }
                                    }

                                    for (int i = 0; i < n_comandos; i++) {
                                        int status;
                                        wait(&status);
                                    }


                                }
                            } else {
                                printf("Comando não reconhecido");
                            }
                        }
                        char remover[] = "remover;";
                        char tamanho[3] = {0};
                        sprintf(tamanho, "%d;", n_filtros);
                        strcat(remover, tamanho);
                        for (int i = 0; i < n_filtros; i++) {
                            char tmp[3] = {0};
                            sprintf(tmp, "%d;", filtros_cliente[i]);
                            strcat(remover, tmp);
                            strcat(remover, ";");
                        }
                        write(fd_client_server_main, remover, sizeof(remover));
                    }

                    close(fd_server_client_id);
                    close(fd_client_server_id);

                    unlink(Server_Client_ID);
                    unlink(Client_Server_ID);
                    //memset(buf, 0, sizeof(buf));

                    kill(getppid(), SIGUSR1);
                    _exit(0);
                }
            }
        }//break;
    }

    int status;
    wait(&status);
    sleep(3);
    printf("bomdia");

    close(fd_client_server_main);
    unlink(Client_Server_Main);
    return 0;
}