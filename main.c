#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <bson.h>
#include <mongoc.h>
#include <setjmp.h>

jmp_buf buferror;

#include "env.c"
#include "processpost.c"
#include "handle.c"

mongoc_client_t *client;
mongoc_database_t *db;
bson_error_t errordb;

static enum MHD_Result controller(void *cls, struct MHD_Connection *connection, const char *url,
                                  const char *method, const char *version, const char *upload_data,
                                  size_t *upload_data_size, void **con_cls)
{
    if (setjmp(buferror) == 0)
    {
        if (NULL == *con_cls)
        {
            struct connection_info_struct *con_info;

            con_info = malloc(sizeof(struct connection_info_struct));
            if (NULL == con_info)
                return MHD_NO;
            con_info->jsonstring = NULL;

            con_info->postprocessor =
                MHD_create_post_processor(connection, POSTBUFFERSIZE,
                                          iterate_post, (void *)con_info);

            if (NULL == con_info->postprocessor)
            {
                free(con_info);
                return MHD_NO;
            }

            con_info->connectiontype = POST;

            *con_cls = (void *)con_info;

            return MHD_YES;
        }
        if (strcmp(method, "POST") == 0)
        {
            struct connection_info_struct *con_info = *con_cls;

            if (*upload_data_size != 0)
            {
                MHD_post_process(con_info->postprocessor, upload_data,
                                 *upload_data_size);
                *upload_data_size = 0;

                return MHD_YES;
            }
            else if (NULL != con_info->jsonstring)
            {
                if (strcmp(url, "/login") == 0)
                {
                    return Login(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/register") == 0)
                {
                    return Register(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/edit-account") == 0)
                {
                    return Edit_account(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/create-post") == 0)
                {
                    return Create_post(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/edit-post") == 0)
                {
                    return Edit_post(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/delete-post") == 0)
                {
                    return Delete_post(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/all-post") == 0)
                {
                    return All_post(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/post") == 0)
                {
                    return Post(connection, con_info->jsonstring, client);
                }
                if (strcmp(url, "/everyone") == 0)
                {
                    return Everyone(connection, con_info->jsonstring, client);
                }
            }
        }
    }
    return send_data(connection, "{\"error\":\"Error for client, please check and try again\"}");
}

int main()
{
    mongoc_init();

    client = mongoc_client_new(uri_string);
    db = mongoc_client_get_database(client, "user");
    if (!client)
    {
        printf("Failed to parse URI.\n");
        return EXIT_FAILURE;
    }

    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_SELECT_INTERNALLY,
                              PORT, NULL, NULL,
                              &controller, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                              NULL, MHD_OPTION_END);

    if (daemon == NULL)
    {
        printf("Không thể khởi tạo máy chủ.\n");
        return 1;
    }

    printf("Máy chủ đang chạy trên cổng %d...\n", PORT);

    getchar();
    mongoc_database_destroy(db);
    mongoc_client_destroy(client);
    MHD_stop_daemon(daemon);
    return 0;
}
