#include <libwebsockets.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include "env.c"

#define MAX_CLIENTS 40

typedef struct
{
    char mail[100];
    struct lws *client;
} ChatClients;

ChatClients Clients[MAX_CLIENTS];

static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        printf("Client connected \n");
        break;
    case LWS_CALLBACK_RECEIVE:
        json_object *data_parse_from_client = json_tokener_parse((char *)in);
        json_object *status_obj;
        json_object_object_get_ex(data_parse_from_client, "status", &status_obj);
        int status = json_object_get_int(status_obj);
        json_object *from_obj;
        json_object_object_get_ex(data_parse_from_client, "from", &from_obj);
        const char *from = json_object_get_string(from_obj);
        // status 0 is connect, 1 is message
        // save client
        if (status == 0)
        {
            for (int i = 1; i < MAX_CLIENTS; i++)
            {
                if (Clients[i].client == NULL)
                {
                    Clients[i].client = wsi;
                    strcpy(Clients[i].mail, from);
                    break;
                }
            }
            printf("connect\n");
        }
        else
        {
            json_object *to_obj;
            json_object_object_get_ex(data_parse_from_client, "to", &to_obj);
            const char *to = json_object_get_string(to_obj);
            json_object *data_obj;
            json_object_object_get_ex(data_parse_from_client, "data", &data_obj);
            const char *data = json_object_get_string(data_obj);
            for (int i = 1; i < MAX_CLIENTS; i++)
            {
                if (strcmp(to, Clients[i].mail) == 0)
                {
                    char response[500];
                    sprintf(response, "{\"status\":\"message\",\"from\":\"%s\",\"data\":\"%s\"}", from, data);
                    lws_write(Clients[i].client, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                    // printf("%s\n", Clients[i].mail);
                    break;
                }
            }
            printf("message\n");
        }
        printf("Received data: %d\n", status);
        break;
    case LWS_CALLBACK_CLOSED:
        // delete client in store
        for (int i = 1; i < MAX_CLIENTS; i++)
        {
            if (Clients[i].client == wsi)
            {
                Clients[i].client = NULL;
                strcpy(Clients[i].mail, "");
                break;
            }
        }
        printf("Client disconnected\n");
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"ws", callback_ws, 0, 0},
    {NULL, NULL, 0, 0}};

int main(int argc, char const *argv[])
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    const char *interface = NULL;
    int opts = 0;

    memset(&info, 0, sizeof info);
    info.port = PORTSK;
    info.iface = interface;
    info.protocols = protocols;
    info.options = opts;

    context = lws_create_context(&info);

    if (context == NULL)
    {
        fprintf(stderr, "Error creating libwebsocket context\n");
        return 1;
    }

    printf("Server started on port %d...\n", PORTSK);

    while (1)
    {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);

    return 0;
}
