
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
 
#define MAX_CLIENTS     100
 
static unsigned int cli_count = 0;
static int uid = 10;
pthread_mutex_t lock;

/* Client structure */
typedef struct {
    struct sockaddr_in addr;    /* Client remote address */
    int connfd;                 /* Connection file descriptor */
    int uid;                    /* Client unique identifier */
    char name[32];              /* Client name */
    char client_time[64];       /* Connect time */
} client_t;
 
client_t *clients[MAX_CLIENTS];//an array of alive clients(alive clients list)
client_t *dead_clients[MAX_CLIENTS];//dead clients list

/* djb2 hash function - used to hash IP+Port into a unique ID */
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
 
/* Add client to queue */
void queue_add(client_t *cl){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(!clients[i]){
            clients[i] = cl;
            return;
        }
    }
}

/* Add dead_client to dead_list */
void dead_list_add(client_t *cl){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(!dead_clients[i]){
            dead_clients[i] = cl;
            return;
        }
    }
}
 
/* Delete client from queue */
void queue_delete(int uid){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                return;
            }
        }
    }
}

/* Delete dead client from dead list (alive again)*/
void dead_list_delete(int uid){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(dead_clients[i]){
            if(dead_clients[i]->uid == uid){
                dead_clients[i] = NULL;
                return;
            }
        }
    }
}
 
/* Send message to all clients but the sender */
void send_message(char *s, int uid){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            if(clients[i]->uid != uid){
                write(clients[i]->connfd, s, strlen(s));
            }
        }
    }
}
 
/* Send message to all clients */
void send_message_all(char *s){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            write(clients[i]->connfd, s, strlen(s));
        }
    }
}
 
/* Send message to sender */
void send_message_self(const char *s, int connfd){
    write(connfd, s, strlen(s));
}
 
/* Send message to client */
void send_message_client(char *s, int uid){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                write(clients[i]->connfd, s, strlen(s));
            }
        }
    }
}

void send_active_clients(int connfd){
    int i;
    char s[64];
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            sprintf(s, "<<CLIENT %d | %s\r\n", clients[i]->uid, clients[i]->name);
            send_message_self(s, connfd);
        }
    }
}
 
/* Strip CRLF */
void strip_newline(char *s){
    while(*s != '\0'){
        if(*s == '\r' || *s == '\n'){
            *s = '\0';
        }
        s++;
    }
}
 



/* Print ip address */
void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xFF,
        (addr.sin_addr.s_addr & 0xFF00)>>8,
        (addr.sin_addr.s_addr & 0xFF0000)>>16,
        (addr.sin_addr.s_addr & 0xFF000000)>>24);
}

void record_conn_time(char *cl_time){
    time_t t = time(0);
    char tmp[64];
    strftime( tmp, sizeof(tmp), "%Y/%m/%d %X %A %z",localtime(&t) );
    int k;
    for(k = 0; k < 64; k++)
        cl_time[k]=tmp[k];
}

/* Print all the client, including dead client.*/
void print_table(){
    int i, j;
    printf("\n***************************\n");
    printf("**    Active Clients:    **\n");
    printf("***************************\n");
    printf("ID        IP          Last Connect Time\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    
    
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            printf("%d            ",clients[i]->uid);
            printf("%s",clients[i]->client_time);
            printf("\n");
        }
    }

    for(i=0;i<MAX_CLIENTS;i++){
        if(dead_clients[i]){
            printf("%d        %s    \n",dead_clients[i]->uid/*client_time[j],*/);
        }
    }
   
    
    printf("\n");

}

void *server_send_command(void *arg){
    char buff_in[1024];
    char buff_out[1024];
    int slen;
    while(1){
        scanf("%s", buff_in);
        slen = strlen(buff_in);
        buff_in[slen] = '\0';
        if(buff_in[0] != '\0'){
            pthread_mutex_lock(&lock);
            sprintf(buff_out, "[Server] %s\r\n", buff_in);
            send_message_all(buff_out);
            buff_in[0] = '\0';
            buff_out[0] = '\0';
            pthread_mutex_unlock(&lock);
        }
       
    }
}
 
/* Handle all communication with the client */
void *hanle_client(void *arg){
    char buff_out[1024];
    char buff_in[1024];
    int rlen;
    
    cli_count++;
    client_t *cli = (client_t *)arg;
    

    printf("<<ACCEPT ");
    print_client_addr(cli->addr);
    record_conn_time(cli->client_time);
    printf(" REFERENCED BY %d\n", cli->uid);
 
    sprintf(buff_out, "<<JOIN, HELLO %s\r\n", cli->name);
    send_message_all(buff_out);
 
    /* Receive input from client */
    while((rlen = read(cli->connfd, buff_in, sizeof(buff_in)-1)) > 0){
        pthread_mutex_lock(&lock);
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        strip_newline(buff_in);
 
        /* Ignore empty buffer */
        if(!strlen(buff_in)){
            continue;
        }
        
        if(!strcmp(buff_in, "CC")){
            //sprintf(buff_out, "<<CLIENTS %d\r\n", cli_count);
            //send_message_self(buff_out, cli->connfd);
            //send_active_clients(cli->connfd);

            print_table();
        }

        /* Special options */
        /*if(buff_in[0] == '\\'){
            char *command, *param;
            command = strtok(buff_in," ");
            if(!strcmp(command, "\\QUIT")){
                break;
            }else if(!strcmp(command, "\\PING")){
                send_message_self("<<PONG\r\n", cli->connfd);
            }else if(!strcmp(command, "\\NAME")){
                param = strtok(NULL, " ");
                if(param){
                    char *old_name = strdup(cli->name);
                    strcpy(cli->name, param);
                    sprintf(buff_out, "<<RENAME, %s TO %s\r\n", old_name, cli->name);
                    free(old_name);
                    send_message_all(buff_out);
                }else{
                    send_message_self("<<NAME CANNOT BE NULL\r\n", cli->connfd);
                }
            }else if(!strcmp(command, "\\PRIVATE")){
                param = strtok(NULL, " ");
                if(param){
                    int uid = atoi(param);
                    param = strtok(NULL, " ");
                    if(param){
                        sprintf(buff_out, "[PM][%s]", cli->name);
                        while(param != NULL){
                            strcat(buff_out, " ");
                            strcat(buff_out, param);
                            param = strtok(NULL, " ");
                        }
                        strcat(buff_out, "\r\n");
                        send_message_client(buff_out, uid);
                    }else{
                        send_message_self("<<MESSAGE CANNOT BE NULL\r\n", cli->connfd);
                    }
                }else{
                    send_message_self("<<REFERENCE CANNOT BE NULL\r\n", cli->connfd);
                }
            }else if(!strcmp(command, "\\ACTIVE")){
                sprintf(buff_out, "<<CLIENTS %d\r\n", cli_count);
                send_message_self(buff_out, cli->connfd);
                send_active_clients(cli->connfd);
            }else if(!strcmp(command, "\\HELP")){
                strcat(buff_out, "\\QUIT     Quit chatroom\r\n");
                strcat(buff_out, "\\PING     Server test\r\n");
                strcat(buff_out, "\\NAME     <name> Change nickname\r\n");
                strcat(buff_out, "\\PRIVATE  <reference> <message> Send private message\r\n");
                strcat(buff_out, "\\ACTIVE   Show active clients\r\n");
                strcat(buff_out, "\\HELP     Show help\r\n");
                send_message_self(buff_out, cli->connfd);
            }else{
                send_message_self("<<UNKOWN COMMAND\r\n", cli->connfd);
            }
        }*/else{
            /* Send message */
            sprintf(buff_out, "[%s] %s\r\n", cli->name, buff_in);
            send_message(buff_out, cli->uid);

            pthread_mutex_unlock(&lock);
        }
    }
 
    /* Close connection */
    close(cli->connfd);
    sprintf(buff_out, "<<LEAVE, BYE %s\r\n", cli->name);
    send_message_all(buff_out);
    

    /* DEAD_Client settings */
    client_t *dead_cli = (client_t *)arg;
    dead_cli->addr = cli->addr;
    dead_cli->connfd = cli->connfd;
    dead_cli->uid = cli->uid;
    sprintf(dead_cli->name, "%d", dead_cli->uid);
    dead_list_add(dead_cli);//add to dead_list

    /* Delete client from queue and yeild thread */
    queue_delete(cli->uid);
    printf("<<LEAVE ");
    print_client_addr(cli->addr);
    printf(" REFERENCED BY %d\n", cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());
}
 
int main(int argc, char *argv[]){
    int listenfd = 0, connfd = 0, n = 0, flag= 1;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid[2];
 
    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10000); 

    /* Set socket option */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    /* Bind */
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Socket binding failed");
        return 1;
    }
 
    /* Listen */
    if(listen(listenfd, 10) < 0){
        perror("Socket listening failed");
        return 1;
    }
 
    printf("<[SERVER STARTED]>\n");

    /* Initial the mutex for threads */
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    /* Create a thread for server to send command */
    pthread_create(&tid[0], NULL, &server_send_command, NULL);

    /* Accept clients */
    while(1){


        int clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
 
        /* Check if max clients is reached */
        if((cli_count+1) == MAX_CLIENTS){
            printf("<<MAX CLIENTS REACHED\n");
            printf("<<REJECT ");
            print_client_addr(cli_addr);
            printf("\n");
            close(connfd);
            continue;
        }
 



        /* Combine IP and Port. Hash it into a unique uid. */
        int Rewrite=0;
        char str[INET_ADDRSTRLEN];
        char hash_str[25];
        char port_str[6];

        inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, str, sizeof(cli_addr));//put dots on client addr(unsigned long int) in order to see more clearly
        sprintf(port_str, "%d", ntohs(cli_addr.sin_port)); //port number: int to string
        strncpy(hash_str, str, 16);//put IP string into hash_str
        strcat(hash_str,port_str);//Combine hash_str(IP) and port string
        /*for(j = 0; ID[j]!=0; j++){
            if(ID[j] == hash(hash_str)){//if there is a same hash ID already existed (but dead)
                ID[j] = hash(hash_str);//Rewrite as alive
                Rewrite = 1;//flag = 1
                break;
            }
        }
        if(Rewrite == 0) ID[j] = hash(hash_str); //New hash ID

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->uid = hash(hash_str);
        sprintf(cli->name, "%d", cli->uid);




        /* Add client to the queue and fork thread */
        queue_add(cli);
        pthread_create(&tid[1], NULL, &hanle_client, (void*)cli);
 
        /* Reduce CPU usage */
        sleep(1);
     }
}