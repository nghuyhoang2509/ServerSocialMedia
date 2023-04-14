#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <pthread.h>

#define LENGTH_MAIL 100
#define LENGTH_MSG 1024
#define MAX_CLIENT 40

struct Client
{
    int data;
    char ip[16];
    char mail[LENGTH_MAIL];
};

struct Client ClientList[MAX_CLIENT];

// Global variables
int server_sockfd = 0, client_sockfd = 0;

void catch_ctrl_c_and_exit(int sig)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        strcpy(ClientList[i].ip, "NULL");
        strcpy(ClientList[i].mail, "");
        close(ClientList[i].data);
        ClientList[i].data = -1;
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void *client_handler(void *i)
{
    int leave_flag = 0;
    char mail[LENGTH_MAIL] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_MSG] = {};
    int index = *(int *)i;

    // set mail
    if (recv(ClientList[index].data, mail, LENGTH_MAIL, 0) <= 0 || strlen(mail) < 2 || strlen(mail) >= LENGTH_MAIL - 1)
    {
        printf("%s didn't input mail.\n", ClientList[index].ip);
        leave_flag = 1;
    }
    else
    {
        strncpy(ClientList[index].mail, mail, LENGTH_MAIL);
        printf("%s %s %d join the server.\n", ClientList[index].mail, ClientList[index].ip, ClientList[index].data);
        sprintf(send_buffer, "success connect index=%d", index);
        send(ClientList[index].data, send_buffer, LENGTH_MSG, 0);
    }

    while (1)
    {
        if (leave_flag)
        {
            break;
        }
        int receive = recv(ClientList[index].data, recv_buffer, LENGTH_MSG, 0);
        json_object *data_parse_from_client = json_tokener_parse(recv_buffer);
        json_object *status_obj;
        json_object_object_get_ex(data_parse_from_client, "status", &status_obj);
        int status = json_object_get_int(status_obj);
        if (status == 1 && receive > 0)
        {
            json_object *from_obj;
            json_object *to_obj;
            json_object *data_obj;
            json_object_object_get_ex(data_parse_from_client, "from", &from_obj);
            json_object_object_get_ex(data_parse_from_client, "to", &to_obj);
            json_object_object_get_ex(data_parse_from_client, "data", &data_obj);
            const char *to = json_object_get_string(to_obj);
            const char *data = json_object_get_string(data_obj);
            const char *from = json_object_get_string(from_obj);
            // finding client
            for (int i = 0; i < MAX_CLIENT; i++)
            {
                if (ClientList[i].data >= 0 && strstr(ClientList[i].mail, to) != NULL)
                {
                    sprintf(send_buffer, "{\"status\":\"1\",\"from\":\"%s\",\"to\":\"%s\",\"data\":\"%s\"}", from, to, data);
                    if (send(ClientList[i].data, send_buffer, LENGTH_MSG, 0) < 0)
                    {
                        send(ClientList[index].data, "{\"status\":\"0\"}", LENGTH_MSG, 0);
                    }
                    break;
                }
                if (i == MAX_CLIENT - 1)
                {
                    send(ClientList[index].data, "{\"status\":\"2\"}", 15, 0);
                }
            }
        }
        else if (receive == 0 || (status == 0 && receive > 0))
        {
            printf("%s %s %d  leave the chatroom.\n", ClientList[index].mail, ClientList[index].ip, ClientList[index].data);
            leave_flag = 1;
        }
        else
        {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
    }

    // Remove
    close(ClientList[index].data);
    strcpy(ClientList[index].ip, "NULL");
    strcpy(ClientList[index].mail, "");
    ClientList[index].data = -1;
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    // initial list
    for (int i = 0; i <= MAX_CLIENT; i++)
    {
        strcpy(ClientList[i].ip, "NULL");
        ClientList[i].data = -1;
    }

    // Create socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sockfd == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    // Socket information
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(9001);

    // Bind and Listen
    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    // Print Server IP
    getsockname(server_sockfd, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);

    printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    while (1)
    {
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);

        // Print Client IP
        getpeername(client_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (strcmp(ClientList[i].ip, "NULL") == 0)
            {
                char ip[16];
                sprintf(ip, "%s:%d", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));
                strcpy(ClientList[i].ip, ip);
                ClientList[i].data = client_sockfd;
                pthread_t id;
                int *index = malloc(sizeof(int));
                *index = i;
                if (pthread_create(&id, NULL, client_handler, index) != 0)
                {
                    send(client_sockfd, "Please try again", LENGTH_MSG, 0);
                }
                break;
            }
            if (i == MAX_CLIENT - 1)
            {
                send(client_sockfd, "Server not available, waitting and try again", LENGTH_MSG, 0);
            }
        }
    }

    return 0;
}