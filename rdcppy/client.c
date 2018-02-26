#include <libgen.h>
#include <fcntl.h>
#include <string.h>

#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <zconf.h>

#include "common.h"
#include "messages.h"
#include "read_cfg.h"

struct client_context {
    char *buffer;
    struct ibv_mr *buffer_mr;

    struct message *msg;
    struct ibv_mr *msg_mr;

    uint64_t peer_addr;
    uint32_t peer_rkey;

    int fd;
    const char *file_name;
    int file_or_dir;
};

static void on_pre_conn(struct rdma_cm_id *id);

static void post_receive(struct rdma_cm_id *id);

static void on_completion(struct ibv_wc *wc);

static void send_file_name(struct rdma_cm_id *id);

static void write_remote(struct rdma_cm_id *id, uint32_t len);

static void send_next_chunk(struct rdma_cm_id *id);

static void send_next_chunk_origin(struct rdma_cm_id *id);

static void usage();

static void rdma_files();

#define MAX_FILE_NUMBER 10240

#define FILE_NAME_LENGTH 256

// local directory's send files name
char files[MAX_FILE_NUMBER][FILE_NAME_LENGTH];

// preprocessiong get remote files name
char target_files[MAX_FILE_NUMBER][FILE_NAME_LENGTH];

// preprocessiong get local files type
short file_or_dir[MAX_FILE_NUMBER];

static int files_count = 0;

int current_index = 0;

static void read_dir(char *dir_name);

static void right(char *dst, char *src, int n);

static int buffer_size = 0;

//#define _DEBUG 1

int main(int argc, char *argv[]) {
    if(argc < 3){
        usage();
        return -1;
    }
    const char* conf_file;
    char *local_dir;
    int is_send_dir = 0;
    int is_config_file = 0;
    int is_bs = 0;

    int arg = 0;
    int bs_value = 0;

// 处理执行指令时带的参数
    for(; arg < argc; arg++){
        if(argv[arg][0] == '-'){
            if(argv[arg][1] == 'd'){ //-d option
                local_dir = argv[++arg];
                is_send_dir = 1;
            }else if(argv[arg][1] == 'b'){
                bs_value = atoi(argv[++arg]);
                is_bs = 1;
            }else if(argv[arg][1] == 'c'){
                conf_file = argv[++arg];
                is_config_file = 1;
            }else if(argv[arg][1] > 'd'){
                printf("Not valid argument\n");
                usage();
                return -1;
            }
        }
    }


//读取配置文件
    struct rcp_config *conf = NULL;
    if(is_config_file){
        conf = config_read(conf_file);
    }else { // no config file
        conf = (struct rcp_config *) calloc(1, sizeof(*conf));;
//        printf("test: %s\n", argv[argc-1]);
        char *split_index = strstr(argv[argc-1], ":/");
        split_index++;

        int len = strcspn(argv[argc-1], ":/");
        strncpy(conf->remote_ip, argv[argc-1], len);
        strcpy(conf->target_dir, split_index);

        if(!is_bs){ // no -b
            conf->bs = BUFFER_SIZE_DEFAULT;
        }else{
            conf->bs = bs_value;
        }
    }

    if (conf == NULL)
    {
        printf("failed to read file: %s\n", conf_file);
        return -1;
    }
//    printf("remote_ip: %s target_dir: %s bs: %d\n", conf->remote_ip, conf->target_dir, conf->bs);


    rdma_files();


    /*char tmp_relative_path[FILE_NAME_LENGTH];
    if(is_send_dir){//-d send a dir

//read_dir函数根据指定的目录读出所有的文件名
        read_dir(local_dir);

        int len = strlen(local_dir);
        int i;
        for (i = 0; i < files_count; i++) {
            right(tmp_relative_path, files[i], len);
            strcpy(target_files[i], conf->target_dir);
            strcat(target_files[i], tmp_relative_path);
            if (file_or_dir[i] == 100)
                strcat(target_files[i], "/");
            printf("%d, %s, %s\n", file_or_dir[i], files[i], target_files[i]);
        }
    }else{//send one regular file
        getcwd(files[0], FILE_NAME_LENGTH);
        strcat(files[0], "/");
        strcat(files[0], argv[1]);
//        printf("send file name is %s\n", files[0]);
        file_or_dir[0] = 101;
        strcpy(target_files[0], conf->target_dir);
        strcat(target_files[0], argv[1]);
    }*/


    buffer_size = conf->bs;
    if(buffer_size <= 0 || buffer_size > BUFFER_SIZE_MAX){
#ifdef _DEBUG
        printf("buffer size error\n");
#endif
        return -1;
    }

    struct client_context ctx;//user define

    rc_init(on_pre_conn, NULL, on_completion, NULL);//common.c

    printf("RDMA is sending files, please wait a moment!\n");

    rc_client_loop(conf->remote_ip, DEFAULT_PORT, &ctx);

    close(ctx.fd);

    printf("ALL files were sent to %s:%s\n", conf->remote_ip, conf->target_dir);
    return 0;

}


void rdma_files(){
    FILE *fp_local;
    FILE *fp_remote;

    char line[1024];
    if((fp_local = fopen("/tmp/rdma_files_local", "r")) == NULL){
        printf("open file error\n");
    }
    if((fp_remote = fopen("/tmp/rdma_files_remote", "r")) == NULL){
        printf("open file error\n");
    }
    int str_length = 0;
    int index = 0;
    while(!feof(fp_local)){
        fgets(line, 1024, fp_local);
        strncpy(files[index], line, strlen(line)-1);
        index ++;
        file_or_dir[index] = 101;
    }
    index = 0;
    while(!feof(fp_remote)){
        fgets(line, 1024, fp_remote);
        strncpy(target_files[index], line, strlen(line)-1);
        index ++;
    }

    int i = 0;
    for(; i < index; i++){
        printf("%s %s", files[i], target_files[i]);
    }

}

void on_pre_conn(struct rdma_cm_id *id) {
#ifdef _DEBUG
    printf("in on_pre_conn\n");
#endif
    struct client_context *ctx = (struct client_context *) id->context;

    posix_memalign((void **) &ctx->buffer, sysconf(_SC_PAGESIZE), buffer_size);
    TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(),
                                       ctx->buffer,
                                       buffer_size,
                                       IBV_ACCESS_LOCAL_WRITE));

    posix_memalign((void **) &ctx->msg, sysconf(_SC_PAGESIZE), sizeof(*ctx->msg));

    TEST_Z(ctx->msg_mr = ibv_reg_mr(rc_get_pd(),
                                    ctx->msg,
                                    sizeof(*ctx->msg),
                                    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

    post_receive(id);

#ifdef _DEBUG
    printf("out on_pre_conn\n");
#endif
}

void write_remote(struct rdma_cm_id *id, uint32_t len) {
    struct client_context *ctx = (struct client_context *) id->context;

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uintptr_t) id;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.imm_data = htonl(len);
    wr.wr.rdma.remote_addr = ctx->peer_addr;
    wr.wr.rdma.rkey = ctx->peer_rkey;

//len是文件名的长度，这个判断会成立
    if (len) {
        wr.sg_list = &sge;
        wr.num_sge = 1;
        //ctx->buffer放的就是需要传输的内容
        sge.addr = (uintptr_t) ctx->buffer;
        sge.length = len;
        sge.lkey = ctx->buffer_mr->lkey;
    }

    TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
}

void post_receive(struct rdma_cm_id *id) {
    struct client_context *ctx = (struct client_context *) id->context;

    struct ibv_recv_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t) id;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = (uintptr_t) ctx->msg;
    sge.length = sizeof(*ctx->msg);
    sge.lkey = ctx->msg_mr->lkey;

    TEST_NZ(ibv_post_recv(id->qp, &wr, &bad_wr));
}

//tag by zhao: 这是关键的函数，发送文件名和发送文件块都是这个函数处理的
char unique_file_name[FILE_NAME_LENGTH];

void on_completion(struct ibv_wc *wc) {
#ifdef _DEBUG
    printf("in on_completion\n");
#endif
    struct rdma_cm_id *id = (struct rdma_cm_id *) (uintptr_t) wc->wr_id;
    struct client_context *ctx = (struct client_context *) id->context;

    if (wc->opcode & IBV_WC_RECV) {
        if (ctx->msg->id == MSG_MR) {
            ctx->peer_addr = ctx->msg->data.mr.addr;
            ctx->peer_rkey = ctx->msg->data.mr.rkey;//第一个程序是lkey
#ifdef _DEBUG
            printf("received MR, now start send file name: %s\n", files[current_index]);
#endif
            if (file_or_dir[current_index] == 100) {//dir

//                ctx->file_or_dir = 100;
            } else {//regular file
//                ctx->file_or_dir = 101;
                ctx->fd = open(files[current_index], O_RDONLY);
            }
            ctx->file_name = target_files[current_index];
            send_file_name(id);
        } else if (ctx->msg->id == MSG_READY) {
//add by zhao: 检查server端什么时候设置了MSG_READY
#ifdef _DEBUG
            printf("received server READY, sending chunk\n");
#endif
            send_next_chunk_origin(id);
        } else if (ctx->msg->id == MSG_DONE) {
#ifdef _DEBUG
            printf("received DONE, disconnecting\n");
#endif
            rc_disconnect(id);
            return;
        } else if (ctx->msg->id == MSG_NEXT) {
#ifdef _DEBUG
            printf("received NEXT message\n");
            printf("sent next file's name\n");
#endif
//tag by zhao: is it necessary
            close(ctx->fd);
            current_index++;
            if (file_or_dir[current_index] == 100) { //dir
                ctx->file_or_dir = 100;

            } else { //regular file
                ctx->fd = open(files[current_index], O_RDONLY);
                ctx->file_or_dir = 101;
            }

            ctx->file_name = target_files[current_index];

            send_file_name(id);
//            send_next_chunk(id);
        }
//tag by zhao: 这个函数是什么作用？？？？
        post_receive(id);
    }
#ifdef _DEBUG
    printf("on_completion end\n");
#endif
}

void send_file_name(struct rdma_cm_id *id) {
    struct client_context *ctx = (struct client_context *) id->context;
#ifdef _DEBUG
    printf("sending file name: %s\n", ctx->file_name);
#endif
    strcpy(ctx->buffer, ctx->file_name);
    if(ctx->file_name[0])
        printf("Sending %s\n", files[current_index]);
    write_remote(id, strlen(ctx->file_name) + 1);


}

void send_next_chunk_origin(struct rdma_cm_id *id) {
    struct client_context *ctx = (struct client_context *) id->context;

    ssize_t size = 0;

    size = read(ctx->fd, ctx->buffer, buffer_size);


    if (size == -1)
        rc_die("read() failed\n");

    write_remote(id, size);
}


void usage()
{
    fprintf(stderr, "usage:   rdcp [-d] file1 [-b block_size] [-c config_file] [remote_ip:]file2\n\n");
    fprintf(stderr, "e.g. :   rdcp -d /home/user1/dir1/ -b 1024 192.168.0.110:/home/user2/dir2/\n");
    fprintf(stderr, "     :   rdcp /home/user1/file1/ -b 1024 -c /home/user/rcp.cfg\n");
}



void read_dir(char *dir_name)
{
    char buf[128];
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    if ((dp = opendir(dir_name)) == NULL) {
        fprintf(stderr, "error while open directory %s\n", dir_name);
        return;
    }
    chdir(dir_name);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        getcwd(buf, sizeof(buf));
        strcat(buf, "/");
        strcat(buf, entry->d_name);
//        printf("%s %s\n", entry->d_name, buf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            file_or_dir[files_count] = 100; //dir
            strcpy(files[files_count], buf);
            files_count++;
            read_dir(entry->d_name);

        } else {
            strcpy(files[files_count], buf);
            file_or_dir[files_count] = 101; //regular file
            files_count++;
        }

    }
    chdir("..");
    closedir(dp);
}


void right(char *dst, char *src, int n)
{
    char *p = src;
    char *q = dst;
    int len = strlen(src);
    if (n > len)
        n = len;
    p += n;
    while ((*(q++) = *(p++)));
    return;
}


void send_next_chunk(struct rdma_cm_id *id) {

    char buf[1024];
    struct client_context *ctx = (struct client_context *) id->context;
    ssize_t size = 0;
    //size = read(ctx->fd, ctx->buffer, BUFFER_SIZE);
#ifdef DEBUG
    printf("send chunk: %s\n", ctx->buffer);
#endif
    fgets(buf, sizeof(buf), stdin);
    size = strlen(buf);

    buf[strlen(buf)] = '\0';
    //*ctx->buffer = 'x';
    strcpy(ctx->buffer, buf);
    //memcpy(&ctx->buffer, buf, strlen(buf));


    if (size == -1)
        rc_die("send chunk: read error");

    printf("next chunk: %ld, %ld\n", size, strlen(ctx->buffer));
    if (strcmp(buf, "exit\n") == 0)
        size = 0;
    write_remote(id, size);
}
