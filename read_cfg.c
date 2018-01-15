//
// Created by lab1 on 18-1-6.
//

#include "read_cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct rcp_config *config_default(void)
{

    struct rcp_config *conf = NULL;

    conf = (struct rcp_config *) calloc(1, sizeof(*conf));
    if (conf == NULL) {
        return NULL;
    }

//if configration file do not define target dir.
//default target dir is "/tmp/" on remote machine.
    strcpy(conf->target_dir, "/tmp/");

//and default block size if 1MB
    conf->bs = 1024 * 1024;

    return conf;
}


int config_defaults(struct rcp_config *conf, const char *buf, char *pos)
{
    if (strcmp(buf, "remote_ip") == 0) {
        strncpy(conf->remote_ip, pos, sizeof(conf->remote_ip));
    } else if (strcmp(buf, "bs") == 0) {
        int val = atoi(pos);
        conf->bs = val;
    } else if (strcmp(buf, "target") == 0) {
//        int val = atoi(pos);
//        conf->beacon_int = val;
        strncpy(conf->target_dir, pos, sizeof(conf->remote_ip));
    } else if (strcmp(buf, "") == 0) {
        printf("please check your configure file.");
        cfg_help();
    }
    // more ...
    return 0;
}

struct rcp_config *config_read(const char *fname)
{
    struct rcp_config *conf = NULL;
    FILE *f = NULL;
    char buf[1024] = {0};
    char *pos = NULL;
    int line = 0;
    int errors = 0;

    f = fopen(fname, "r");
    if (f == NULL) {
        printf("Could not open configuration file '%s' for reading.\n", fname);
        return NULL;
    }else{
#ifdef _DEBUG
        printf("configuration file '%s' opened.\n", fname);
#endif

    }

    conf = config_default();
    if (conf == NULL) {
        fclose(f);
        return NULL;
    }

    while (fgets(buf, sizeof(buf), f)) {
        line++;

        if (buf[0] == '#')
            continue;
        pos = buf;
        while (*pos != '\0') {
            if (*pos == '\n') {
                *pos = '\0';
                break;
            }
            pos++;
        }
        if (buf[0] == '\0')
            continue;

        pos = strchr(buf, '=');
        if (pos == NULL) {
            printf("Line %d: invalid line '%s'\n", line, buf);
            errors++;
            continue;
        }
        *pos = '\0';
        pos++;
        errors += config_defaults(conf, buf, pos);
    }

    fclose(f);

    return conf;
}


int check_path(char *fname)
{
    struct rcp_config *conf = NULL;
    FILE *f = NULL;
    f = fopen(fname, "r");
    if (f == NULL) {
        printf("Could not open configuration file '%s' for reading.\n", fname);
        conf = config_default();
        if (conf == NULL) {
            fclose(f);
            cfg_help();
//            fprintf(stderr, "example configure file:\n\n");
//            fprintf(stderr, "remote_ip=192.168.0.110");
//            fprintf(stderr, "target=/home/lab2/files/");
//            fprintf(stderr, "bs=10485760");
            return -1;
        }
        return -1;
    }else{
#ifdef _DEBUG
        printf("configuration file '%s' opened.\n", fname);
#endif
        return 0;
    }
}

void cfg_help()
{
    fprintf(stderr, "example configure file:\n\n");
    fprintf(stderr, "remote_ip=192.168.0.110");
    fprintf(stderr, "target=/home/lab2/files/");
    fprintf(stderr, "bs=10485760");
    return;
}

