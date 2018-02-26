//
// Created by lab1 on 18-1-6.
//

#ifndef RDMA_READ_CFG_H
#define RDMA_READ_CFG_H

#define IP_LENGTH 20
#define FILE_PATH_LENGHT 128
struct rcp_config
{
    // remote ip, first install RDMA and get a ipv4 ip address
    char remote_ip[IP_LENGTH];
    char local_dir[FILE_PATH_LENGHT];
    char target_dir[FILE_PATH_LENGHT];

    // block size
    int bs;
};

struct rcp_config * config_default(void);
int config_defaults(struct rcp_config *conf, const char *buf, char *pos);
struct rcp_config* config_read(const char *fname);

int check_path(char *pos);
void cfg_help();

#endif //RDMA_READ_CFG_H
