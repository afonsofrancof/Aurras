#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define Client_Server_Main  "../etc/client_server_main_fifo"


int main()
{


    int fd_client_server_main;
    char *Client_Server_ID  = "../tmp/client_server_";
    char Server_Client_ID[BUFSIZ]  = "../tmp/server_client_";
    char buf[BUFSIZ];

    /* Criar o pipe principal */
    mkfifo(Client_Server_Main, 0666);
    printf("Server ON.\n");

    fd_client_server_main = open(Client_Server_Main, O_RDONLY);
    while (1)
    {
    /* Abrir o main pipe e ficar à espera que os users se conectem */

        ssize_t tamanho_read;
        tamanho_read=read(fd_client_server_main, buf, BUFSIZ);
        if(tamanho_read>0) {
            if (fork() == 0) {
                char path_server_client_id[BUFSIZ]; strcat(strcpy(path_server_client_id,Server_Client_ID),buf);
                char path_client_server_id[BUFSIZ]; strcat(strcpy(path_client_server_id,Client_Server_ID),buf);

                if(mkfifo(path_server_client_id, 0666)!=-1) printf("Erro creating FIFO");
                if(mkfifo(path_client_server_id, 0666)!=-1) printf("Erro creating FIFO");

                int fd_server_client_id = open(path_server_client_id, O_WRONLY);
                int fd_client_server_id = open(path_client_server_id, O_RDONLY);

                char *con = "Connection Accepted";
                if (write(fd_server_client_id, con, 21) < 0) {
                    perror("Erro na escrita:");
                    _exit(-1);
                }

                close(fd_server_client_id);
                close(fd_client_server_id);

                unlink(path_server_client_id);
                unlink(path_client_server_id);

                memset(buf, 0, sizeof(buf));
                _exit(0);
            } else {
                printf("bomdia");
            }
        }


    }

    return 0;
}