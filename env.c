#define POSTBUFFERSIZE 1024
#define MAXJSONSIZE 1024

#define POST 1

const char *error = "Fail.";
int PORT = 80;
char *IP = "127.0.0.1";
const char *uri_string = "mongodb+srv://nhh2509:MaiOcwqbb2amcf7g@cluster0.7wsgsz6.mongodb.net/?retryWrites=true&w=majority";

struct connection_info_struct
{
    int connectiontype;
    char *jsonstring;
    struct MHD_PostProcessor *postprocessor;
};
