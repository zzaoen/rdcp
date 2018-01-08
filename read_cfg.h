//
// Created by lab1 on 18-1-6.
//

#ifndef RDMA_READ_CFG_H
#define RDMA_READ_CFG_H

struct rcp_config
{
    char remote_ip[20];
    char local_dir[128];
    char target_dir[128];
    int bs;
};

struct rcp_config * config_default(void);
int config_defaults(struct rcp_config *conf, const char *buf, char *pos);
struct rcp_config* config_read(const char *fname);


#endif //RDMA_READ_CFG_H
