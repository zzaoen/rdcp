#ifndef RDMA_MESSAGES_H
#define RDMA_MESSAGES_H

const char *DEFAULT_PORT = "12345";
const size_t BUFFER_SIZE = 400 * 1024 * 1024;
const size_t BUFFER_SIZE_DEFAULT = 10 * 1024 * 1024;
const size_t BUFFER_SIZE_MAX = 100 * 1024 * 1024;

enum message_id {
	MSG_INVALID = 0,
	MSG_MR,
	MSG_READY,
	MSG_NEXT,
	MSG_DONE
};

struct message {
	int id;

	union {
		struct {
			uint64_t addr;
			uint32_t rkey;
		} mr;
	} data;
};

#endif
