
enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                             const char *filename, const char *content_type,
                             const char *transfer_encoding, const char *data, uint64_t off,
                             size_t size)
{
    struct connection_info_struct *con_info = coninfo_cls;
    (void)kind;              /* Unused. Silent compiler warning. */
    (void)filename;          /* Unused. Silent compiler warning. */
    (void)content_type;      /* Unused. Silent compiler warning. */
    (void)transfer_encoding; /* Unused. Silent compiler warning. */
    (void)off;               /* Unused. Silent compiler warning. */

    if (size > 0)
    {
        char *jsonstring;
        jsonstring = malloc(MAXJSONSIZE);
        if (!jsonstring)
            return MHD_NO;

        snprintf(jsonstring, MAXJSONSIZE, "%s", data);
        con_info->jsonstring = jsonstring;
    }
    else
    {
        con_info->jsonstring = NULL;

        return MHD_NO;
    }

    return MHD_YES;
}

enum MHD_Result
send_data(struct MHD_Connection *connection, const char *data)
{
    enum MHD_Result ret;
    struct MHD_Response *response;

    response =
        MHD_create_response_from_buffer(strlen(data), (void *)data,
                                        MHD_RESPMEM_PERSISTENT);
    if (!response)
        return MHD_NO;
    MHD_add_response_header(response, "Content-Type", "application/json");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info_struct *con_info = *con_cls;
    (void)cls;        /* Unused. Silent compiler warning. */
    (void)connection; /* Unused. Silent compiler warning. */
    (void)toe;        /* Unused. Silent compiler warning. */

    if (NULL == con_info)
        return;

    if (con_info->connectiontype == POST)
    {
        MHD_destroy_post_processor(con_info->postprocessor);
        if (con_info->jsonstring)
            free(con_info->jsonstring);
    }

    free(con_info);
    *con_cls = NULL;
}