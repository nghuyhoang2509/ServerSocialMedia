enum MHD_Result Login(struct MHD_Connection *connection, const char *jsonstring, mongoc_collection_t *collection)
{
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
    json_object_put(parsed_json_from_client);

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
                mongoc_cursor_destroy(cursor);
                bson_destroy(query);
                json_object_put(jsmail);
                json_object_put(jspassword);
                return MHD_YES;
            }
            else
            {

                string_res = "{\"error\":\"mail or password incorrect\"}";
                send_data(connection, string_res);
                mongoc_cursor_destroy(cursor);
                bson_destroy(query);
                json_object_put(jsmail);
                json_object_put(jspassword);
                return MHD_YES;
            }
        }
    }
    string_res = "{\"error\":\"mail or password incorrect\"}";
    send_data(connection, string_res);
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    json_object_put(jsmail);
    json_object_put(jspassword);
    return MHD_YES;
}

enum MHD_Result Register(struct MHD_Connection *connection, const char *jsonstring, mongoc_collection_t *collection)
{
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
        bson_destroy(info_register);
        bson_destroy(validate_info);
        mongoc_cursor_destroy(cursor);
        json_object_put(jsmail);
        json_object_put(jsfullname);
        json_object_put(jspassword);
        return send_data(connection, "{\"error\":\"Mail already exists\"}");
    }
    json_object_object_get_ex(parsed_json_from_client, "password", &jspassword);
    json_object_object_get_ex(parsed_json_from_client, "fullname", &jsfullname);
    BSON_APPEND_UTF8(info_register, "password", json_object_get_string(jspassword));
    BSON_APPEND_UTF8(info_register, "fullname", json_object_get_string(jsfullname));
    BSON_APPEND_UTF8(info_register, "mail", json_object_get_string(jsmail));
    json_object_put(parsed_json_from_client);
    if (!mongoc_collection_insert_one(collection, info_register, NULL, NULL, &error))
    {
        bson_destroy(info_register);
        bson_destroy(validate_info);
        mongoc_cursor_destroy(cursor);
        json_object_put(jsmail);
        json_object_put(jsfullname);
        json_object_put(jspassword);
        return send_data(connection, "{\"error\":\"There is a problem, please try again\"}");
    }
    bson_destroy(info_register);
    bson_destroy(validate_info);
    mongoc_cursor_destroy(cursor);
    json_object_put(jsmail);
    json_object_put(jsfullname);
    json_object_put(jspassword);
    return send_data(connection, "{\"success\":\"Register successful\"}");
}

enum MHD_Result Edit_account(struct MHD_Connection *connection, const char *jsonstring, mongoc_collection_t *collection)
{
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
    return MHD_YES;
    json_object_put(parsed_json_from_client);
}