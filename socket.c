#include <libwebsockets.h>
#include <stdio.h>
#include "env.c"

struct chat_node
{
    struct chat_node *next;
    struct lws *user1;
    struct lws *user2;
};

struct client_node
{
    struct client_node *next;
    char *id;
    char *mail;
    struct lws *user;
};

static struct chat_node *chats = NULL;
static struct client_node *clients = NULL;

static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        if (!clients->user)
        {
            clients->next->user = wsi;
        }
        printf("Client connected \n");
        break;
    case LWS_CALLBACK_RECEIVE:
        printf("Received data: %s\n", (char *)in);
        break;

    case LWS_CALLBACK_CLOSED:
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
