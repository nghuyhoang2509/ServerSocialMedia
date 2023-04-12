enum MHD_Result Login(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection;
    collection = mongoc_client_get_collection(client, "user", "account");
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jsmail;
    json_object *jspassword;
    bson_t *query;
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    char *string_res;
    string_res = malloc(MAXJSONSIZE);

    json_object_object_get_ex(parsed_json_from_client, "mail", &jsmail);
    const char *mail = json_object_get_string(jsmail);
    json_object_object_get_ex(parsed_json_from_client, "password", &jspassword);
    const char *password = json_object_get_string(jspassword);

    query = bson_new();
    BSON_APPEND_UTF8(query, "mail", mail);

    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    if (mongoc_cursor_next(cursor, &doc))
    {
        bson_iter_t iter_pass;
        if (bson_iter_init_find(&iter_pass, doc, "password"))
        {
            const char *password_db = bson_iter_utf8(&iter_pass, NULL);
            if (strcmp(password_db, password) == 0)
            {
                string_res = bson_as_json(doc, NULL);
                send_data(connection, string_res);
            }
            else
            {

                string_res = "{\"error\":\"mail or password incorrect\"}";
                send_data(connection, string_res);
            }
        }
        else
        {
            string_res = "{\"error\":\"mail or password incorrect\"}";
            send_data(connection, string_res);
        }
    }
    else
    {
        string_res = "{\"error\":\"mail or password incorrect\"}";
        send_data(connection, string_res);
    }
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    json_object_put(jsmail);
    json_object_put(jspassword);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result Register(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection;
    collection = mongoc_client_get_collection(client, "user", "account");
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jspassword;
    json_object *jsmail;
    json_object *jsfullname;
    bson_t *info_register = bson_new();
    bson_t *validate_info = bson_new();
    const bson_t *doc;
    bson_error_t error;
    mongoc_cursor_t *cursor;

    json_object_object_get_ex(parsed_json_from_client, "mail", &jsmail);

    BSON_APPEND_UTF8(validate_info, "mail", json_object_get_string(jsmail));
    cursor = mongoc_collection_find_with_opts(collection, validate_info, NULL, NULL);
    if (mongoc_cursor_next(cursor, &doc))
    {
        send_data(connection, "{\"error\":\"Mail already exists\"}");
    }
    else
    {
        json_object_object_get_ex(parsed_json_from_client, "password", &jspassword);
        json_object_object_get_ex(parsed_json_from_client, "fullname", &jsfullname);
        BSON_APPEND_UTF8(info_register, "password", json_object_get_string(jspassword));
        BSON_APPEND_UTF8(info_register, "fullname", json_object_get_string(jsfullname));
        BSON_APPEND_UTF8(info_register, "mail", json_object_get_string(jsmail));
        if (!mongoc_collection_insert_one(collection, info_register, NULL, NULL, &error))
        {
            send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
        }
        else
        {

            send_data(connection, "{\"success\":\"Register successful\"}");
        }
    }
    bson_destroy(info_register);
    bson_destroy(validate_info);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result Edit_account(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection;
    collection = mongoc_client_get_collection(client, "user", "account");
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jspassword;
    json_object *jsfullname;
    json_object *js_id;
    bson_oid_t oid;
    bson_t *query = NULL;
    bson_t *update = NULL;
    bson_error_t error;
    json_object_object_get_ex(parsed_json_from_client, "password", &jspassword);
    json_object_object_get_ex(parsed_json_from_client, "fullname", &jsfullname);
    json_object_object_get_ex(parsed_json_from_client, "_id", &js_id);
    bson_oid_init_from_string(&oid, json_object_get_string(js_id));
    query = BCON_NEW("_id", BCON_OID(&oid));
    update = BCON_NEW("$set", "{",
                      "password", BCON_UTF8(json_object_get_string(jspassword)),
                      "fullname", BCON_UTF8(json_object_get_string(jsfullname)),
                      "}", NULL);
    if (!mongoc_collection_update_one(collection, query, update, NULL, NULL, &error))
    {
        send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
    }
    else
    {
        send_data(connection, jsonstring);
    }
    bson_destroy(update);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result Create_post(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *post = mongoc_client_get_collection(client, "user", "post");
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jsuserId;
    json_object *jsmail;
    json_object *jscontent;
    bson_t *doc = bson_new();
    bson_oid_t oid;
    bson_oid_init(&oid, NULL);
    json_object_object_get_ex(parsed_json_from_client, "userId", &jsuserId);
    json_object_object_get_ex(parsed_json_from_client, "mail", &jsmail);
    json_object_object_get_ex(parsed_json_from_client, "content", &jscontent);
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "mail", json_object_get_string(jsmail));
    BSON_APPEND_UTF8(doc, "content", json_object_get_string(jscontent));
    BSON_APPEND_UTF8(doc, "userId", json_object_get_string(jsuserId));
    if (!mongoc_collection_insert_one(post, doc, NULL, NULL, NULL))
    {
        send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
    }
    else
    {
        char str_oid[100];
        bson_oid_to_string(&oid, str_oid);
        char *string_res;
        string_res = malloc(MAXJSONSIZE * 100);
        sprintf(string_res, "{\"success\":\"Create successful\",\"postId\":\"%s\"}", str_oid);
        send_data(connection, string_res);
    }

    bson_destroy(doc);
    mongoc_collection_destroy(post);
    return MHD_YES;
}

enum MHD_Result Edit_post(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection;
    collection = mongoc_client_get_collection(client, "user", "post");
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jscontent;
    json_object *js_id;
    bson_oid_t oid;
    bson_t *query = NULL;
    bson_t *update = NULL;
    bson_error_t error;
    json_object_object_get_ex(parsed_json_from_client, "content", &jscontent);
    json_object_object_get_ex(parsed_json_from_client, "_id", &js_id);
    bson_oid_init_from_string(&oid, json_object_get_string(js_id));
    query = BCON_NEW("_id", BCON_OID(&oid));
    update = BCON_NEW("$set", "{",
                      "content", BCON_UTF8(json_object_get_string(jscontent)),
                      "}", NULL);
    if (!mongoc_collection_update_one(collection, query, update, NULL, NULL, &error))
    {
        send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
    }
    else
    {
        send_data(connection, jsonstring);
    }
    bson_destroy(update);
    bson_destroy(query);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result Delete_post(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "user", "post");
    bson_t *query;
    bson_oid_t oid;

    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *js_id;
    json_object_object_get_ex(parsed_json_from_client, "_id", &js_id);

    query = bson_new();
    bson_oid_init_from_string(&oid, json_object_get_string(js_id));
    BSON_APPEND_OID(query, "_id", &oid);

    if (!mongoc_collection_delete_one(collection, query, NULL, NULL, NULL))
    {
        send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
    }
    else
    {
        send_data(connection, "{\"success\":\"Delete successful\"}");
    }

    bson_destroy(query);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result Everyone(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "user", "account");
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    bson_t *query;
    json_object *jsarraymail = json_object_new_array();
    json_object *jsarrayid = json_object_new_array();

    query = bson_new();

    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    while (mongoc_cursor_next(cursor, &doc))
    {
        bson_iter_t iter_mail;
        bson_iter_t iter_id;
        bson_oid_t oid;
        if (bson_iter_init_find(&iter_id, doc, "_id") && BSON_ITER_HOLDS_OID(&iter_id))
        {
            bson_oid_copy(bson_iter_oid(&iter_id), &oid);
            char *oid_str;
            oid_str = malloc(24 * 2 + 1);
            bson_oid_to_string(&oid, oid_str);
            json_object_array_add(jsarrayid, json_object_new_string(oid_str));
            free(oid_str);
        }

        if (bson_iter_init_find(&iter_mail, doc, "mail"))
        {
            json_object_array_add(jsarraymail, json_object_new_string(bson_iter_utf8(&iter_mail, NULL)));
        }
    }
    char *string_res;
    string_res = malloc(MAXJSONSIZE * 100);
    sprintf(string_res, "{\"mail\":%s, \"userId\":%s}", json_object_to_json_string(jsarraymail), json_object_to_json_string(jsarrayid));
    send_data(connection, string_res);

    free(string_res);
    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    json_object_put(jsarraymail);
    json_object_put(jsarrayid);
    return MHD_YES;
}

enum MHD_Result Post(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "user", "post");
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    bson_t *query;
    json_object *jsarray = json_object_new_array();
    json_object *jstring;
    json_object *parsed_json_from_client = json_tokener_parse(jsonstring);
    json_object *jsuserId;
    json_object_object_get_ex(parsed_json_from_client, "userId", &jsuserId);

    query = bson_new();
    BSON_APPEND_UTF8(query, "userId", json_object_get_string(jsuserId));

    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);
    while (mongoc_cursor_next(cursor, &doc))
    {
        char *json_str = bson_as_json(doc, NULL);
        json_object_array_add(jsarray, jstring = json_object_new_string(json_str));
        bson_free(json_str);
    }
    char *string_res;
    string_res = malloc(MAXJSONSIZE * 100);
    sprintf(string_res, "{\"posts\":%s}", json_object_to_json_string(jsarray));
    send_data(connection, string_res);

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}

enum MHD_Result All_post(struct MHD_Connection *connection, const char *jsonstring, mongoc_client_t *client)
{
    mongoc_collection_t *collection = mongoc_client_get_collection(client, "user", "post");
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    bson_t *query;
    json_object *jsarray = json_object_new_array();
    json_object *jstring;

    query = bson_new();

    cursor = mongoc_collection_find_with_opts(collection, query, NULL, NULL);

    while (mongoc_cursor_next(cursor, &doc))
    {
        char *json_str = bson_as_json(doc, NULL);
        json_object_array_add(jsarray, jstring = json_object_new_string(json_str));
        bson_free(json_str);
    }
    char *string_res;
    string_res = malloc(MAXJSONSIZE * 100);
    sprintf(string_res, "{\"posts\":%s}", json_object_to_json_string(jsarray));
    send_data(connection, string_res);

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return MHD_YES;
}
