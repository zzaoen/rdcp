#include <fcntl.h>
#include <sys/stat.h>

#include "common.h"
#include "messages.h"

#define MAX_FILE_NAME 256

struct conn_context {
	char *buffer;
	struct ibv_mr *buffer_mr;

	struct message *msg;
	struct ibv_mr *msg_mr;

	int fd;
	char file_name[MAX_FILE_NAME];
	int file_or_dir;
};

static void send_message(struct rdma_cm_id *id) {

	struct conn_context *ctx = (struct conn_context *) id->context;

	struct ibv_send_wr wr, *bad_wr = NULL;

	struct ibv_sge sge;

	memset(&wr, 0, sizeof(wr));

	wr.wr_id = (uintptr_t) id;
	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;

	sge.addr = (uintptr_t) ctx->msg;
	sge.length = sizeof(*ctx->msg);
	sge.lkey = ctx->msg_mr->lkey;

	TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
}

static void post_receive(struct rdma_cm_id *id) {
	struct ibv_recv_wr wr, *bad_wr = NULL;

	memset(&wr, 0, sizeof(wr));

	wr.wr_id = (uintptr_t) id;
	wr.sg_list = NULL;
	wr.num_sge = 0;

	TEST_NZ(ibv_post_recv(id->qp, &wr, &bad_wr));
}

static void on_pre_conn(struct rdma_cm_id *id) {
#ifdef _DEBUG
	printf("on_pre_conn\n");
#endif
	struct conn_context *ctx = (struct conn_context *) malloc(sizeof(struct conn_context));

	id->context = ctx;

	ctx->file_name[0] = '\0'; // take this to mean we don't have the file name

	//set up buffer size??
	posix_memalign((void **) &ctx->buffer, sysconf(_SC_PAGESIZE), BUFFER_SIZE);
	TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(),
                                       ctx->buffer,
                                       BUFFER_SIZE,
									   IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

	posix_memalign((void **) &ctx->msg, sysconf(_SC_PAGESIZE), sizeof(*ctx->msg));

	TEST_Z(ctx->msg_mr = ibv_reg_mr(rc_get_pd(),
                                    ctx->msg,
                                    sizeof(*ctx->msg),
									IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

	post_receive(id);
}

static void on_connection(struct rdma_cm_id *id) {
#ifdef _DEBUG
	printf("on_connection\n");
#endif
	struct conn_context *ctx = (struct conn_context *) id->context;

	ctx->msg->id = MSG_MR;
	ctx->msg->data.mr.addr = (uintptr_t) ctx->buffer_mr->addr;
	ctx->msg->data.mr.rkey = ctx->buffer_mr->rkey;

	send_message(id);
}


//execute 2 times, size is the file's size, and the last time size is 0
int FLAG = 1;
static void on_completion(struct ibv_wc *wc) {
	struct rdma_cm_id *id = (struct rdma_cm_id *) (uintptr_t) wc->wr_id;
	struct conn_context *ctx = (struct conn_context *) id->context;

	if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM) {
		uint32_t size = ntohl(wc->imm_data);
#ifdef _DEBUG
		printf("on_completion, size: %d\n", size);
#endif
		if (size == 0) {
#ifdef _DEBUG
			printf("now receive message size is %d\n", size);
            printf("in MSG_NEXT\n");
#endif
            printf("Received file: %s\n", ctx->file_name);
            post_receive(id);
            ctx->msg->id = MSG_NEXT;

            ctx->file_name[0] = 0;
            close(ctx->fd);
            send_message(id);
			// don't need post_receive() since we're done with this connection
		} else if (ctx->file_name[0]) {
//check file_name, if file_name is getted, then do follows; else enter else to save file name
#ifdef _DEBUG
			printf("ctx->file_name: %s\n", ctx->file_name);
#endif
			ssize_t ret;
#ifdef _DEBUG
			printf("received %i bytes.\n", size);
#endif
			ret = write(ctx->fd, ctx->buffer, size);
//			printf("message from client: %s\n", ctx->buffer);
			if (ret != size)
				rc_die("write() failed");
			post_receive(id);
			ctx->msg->id = MSG_READY;
			send_message(id);

		} else {
			memcpy(ctx->file_name, ctx->buffer, (size > MAX_FILE_NAME) ? MAX_FILE_NAME : size);

			ctx->file_name[size - 1] = '\0';

			if(size == 1) {
				ctx->msg->id = MSG_DONE;
			}else if(ctx->file_name[size-2] == '/'){
#ifdef _DEBUG
				printf("this is a dir\n");
#endif
                printf("Created directory %s\n", ctx->file_name);
//				strcpy(ctx->file_name, "dirfile");
				mkdir(ctx->file_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				ctx->file_name[0] = 0;
				ctx->msg->id = MSG_NEXT;

			}else{
#ifdef _DEBUG
				printf("opening file :%s\n", ctx->file_name);
#endif
				ctx->fd = open(ctx->file_name,
							   O_WRONLY | O_CREAT,
							   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

//				if (ctx->fd == -1){
//				rc_die("open() failed");
//					ctx->msg->id = MSG_DONE;
//				}else{
					ctx->msg->id = MSG_READY;
//				}
			}


			post_receive(id);

			send_message(id);
		}
	}
}

static void on_disconnect(struct rdma_cm_id *id) {
#ifdef _DEBUG
	printf("on_disconnect\n");
#endif
	struct conn_context *ctx = (struct conn_context *) id->context;

	close(ctx->fd);

	ibv_dereg_mr(ctx->buffer_mr);
	ibv_dereg_mr(ctx->msg_mr);

	free(ctx->buffer);
	free(ctx->msg);

	printf("All files were received\n\n");

	free(ctx);
}

int main(int argc, char **argv) {
	rc_init(
			on_pre_conn,
			on_connection,
			on_completion,
			on_disconnect
    );

	printf("waiting for connections. interrupt (^C) to exit.\n");

	rc_server_loop(DEFAULT_PORT);

	return 0;
}
