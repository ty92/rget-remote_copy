#define PrtErr(m) { \
        perror(m);  \
        exit(1);    \
        }

#define LISTEN 1

//server 
bool process_request();
bool is_regfile();
bool is_dir();
bool send_filename();
bool send_filedata();

//client
void usage(char **argv);
int recv_file();
int recv_dir();
char *recv_filename();
void recv_filedata();
bool whether_exist(char *filename);
char *nopath(char *filename);
int delete_dir(char *filename);

struct argu_option {
        bool recursive;
        bool force;
};
