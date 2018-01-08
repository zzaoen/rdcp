#include <libgen.h>
#include <fcntl.h>

#include <string.h>

#include "my_common.h"
#include "messages.h"

struct client_context{
    char *buffer;
    struct ibv_mr *buffer_mr;

    struct message *msg;
    struct ibv_mr *msg_mr;

    uint64_t peer_addr;
    uint32_t peer_rkey;

    int fd;
    const char *file_name;
};



static void on_pre_conn(struct rdma_cm_id *id);
    static void post_receive(struct rdma_cm_id *id);
    
static void on_completion(struct ibv_wc *wc);
    static void send_file_name(struct rdma_cm_id *id);
        static void write_remote(struct rdma_cm_id *id, uint32_t len);
    static void send_next_chunk(struct rdma_cm_id *id);
        //static void write_remote
int main(int argc, char *argv[])
{
    struct client_context ctx;//user define
    
    if (argc != 2){
        fprintf(stderr, "usage: %s <server-address> <file-name>\n", argv[0]);
        return 1;
    }

    ctx.file_name = basename(argv[1]);//#include <libgen.h>
    ctx.fd = open(argv[1], O_RDONLY);
    if(ctx.fd == -1){
        printf("open file error\n");
        return 1;
    }

    rc_init(on_pre_conn, NULL, on_completion, NULL);//common.c
// static pre_conn_cb_fn s_on_pre_conn_cb = NULL;
// typedef void (*pre_conn_cb_fn)(struct rdma_cm_id *id);
// static connect_cb_fn s_on_connect_cb = NULL;
// static completion_cb_fn s_on_completion_cb = NULL;
// static disconnect_cb_fn s_on_disconnect_cb = NULL;
// rc_init(pre_conn_cb_fn pc, connect_cb_fn conn, completion_cb_fn comp, disconnect_cb_fn disc)
// static void on_pre_conn(struct rdma_cm_id *id);
    

    //rdma连接的初始化操作，在common.c
    rc_client_loop("192.168.0.7", DEFAULT_PORT, &ctx);
    
    close(ctx.fd);

    return 0;
    
}


void on_pre_conn(struct rdma_cm_id *id)
{   
    printf("in on_pre_conn\n");
    struct client_context *ctx = (struct client_context *)id->context;

    posix_memalign((void **)&ctx->buffer, sysconf(_SC_PAGESIZE), BUFFER_SIZE);
    TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE));

    posix_memalign((void **)&ctx->msg, sysconf(_SC_PAGESIZE), sizeof(*ctx->msg));
    TEST_Z(ctx->msg_mr = ibv_reg_mr(rc_get_pd(), ctx->msg, sizeof(*ctx->msg), IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

    post_receive(id);
    printf("out on_pre_conn\n");
}
void write_remote(struct rdma_cm_id *id, uint32_t len)
{  
    struct client_context *ctx = (struct client_context *)id->context;

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id = (uintptr_t)id;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.imm_data = htonl(len);
    wr.wr.rdma.remote_addr = ctx->peer_addr;
    wr.wr.rdma.rkey = ctx->peer_rkey;

//tag by zhao:len是文件名的长度，这个判断会成立
    if(len){
        wr.sg_list = &sge;
        wr.num_sge = 1;
        //ctx->buffer放的就是需要传输的内容
        sge.addr = (uintptr_t)ctx->buffer;
        sge.length = len;
        sge.lkey = ctx->buffer_mr->lkey;
    }
    TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
}
void post_receive(struct rdma_cm_id *id)
{
    struct client_context *ctx = (struct client_context *)id->context;

    struct ibv_recv_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t)id;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = (uintptr_t)ctx->msg;
    sge.length = sizeof(*ctx->msg);
    sge.lkey = ctx->msg_mr->lkey;

    TEST_NZ(ibv_post_recv(id->qp, &wr, &bad_wr));
}
//tag by zhao: 这是关键的函数，发送文件名和发送文件块都是这个函数处理的
void on_completion(struct ibv_wc *wc)
{
    printf("in on_completion\n");
    struct rdma_cm_id *id = (struct rdma_cm_id *)(uintptr_t)wc->wr_id;
    struct client_context *ctx = (struct client_context *)id->context;

    if(wc->opcode & IBV_WC_RECV){
        if(ctx->msg->id == MSG_MR){
            ctx->peer_addr = ctx->msg->data.mr.addr;
            ctx->peer_rkey = ctx->msg->data.mr.rkey;//第一个程序是lkey
            printf("received MR, now start send file name\n");
            send_file_name(id);
        }else if(ctx->msg->id == MSG_READY){
//add by zhao: 检查server端什么时候设置了MSG_READY
            printf("received server READY, sending chunk\n");
            send_next_chunk(id);
        }else if(ctx->msg->id == MSG_DONE){
            printf("received DONE, disconnecting\n");
            rc_disconnect(id);
            return;
        }
//tag by zhao: 这个函数是什么作用？？？？
        post_receive(id);
    }
    printf("on_completion end\n");
}

void send_file_name(struct rdma_cm_id *id)
{
    printf("sending file name\n");
    struct client_context *ctx = (struct client_context *)id->context;
    strcpy(ctx->buffer, ctx->file_name);

    write_remote(id, strlen(ctx->file_name)+1);
    printf("file name was sent\n");
}


int i = 1;
void send_next_chunk(struct rdma_cm_id *id)
{

    char buf[1024];
    struct client_context *ctx = (struct client_context *)id->context;
    ssize_t size = 0;
    //size = read(ctx->fd, ctx->buffer, BUFFER_SIZE);
    printf("send chunk: %s\n", ctx->buffer);
    
    fgets(buf, sizeof(buf), stdin);
    size = strlen(buf);

    buf[strlen(buf)] = '\0';
    //*ctx->buffer = 'x';
    strcpy(ctx->buffer, buf);
     //memcpy(&ctx->buffer, buf, strlen(buf));


    if(size == -1)
        rc_die("send chunk: read error");
    

    // if(i == 3)
    //     size = 0;
    // i++;
    //write_remote(id, strlen(ctx->buffer));//这种方式循环不停，持续发送文件数据
    printf("next chunk: %ld, %ld\n", size, strlen(ctx->buffer));
    if(strcmp(buf, "exit\n") == 0)
        size = 0;
    write_remote(id, size);
}


