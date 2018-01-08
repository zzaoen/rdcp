//
// Created by lab1 on 18-1-6.
//

#include "read_cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct rcp_config *config_default(void) {
    struct rcp_config *conf = NULL;

    conf = (struct rcp_config *) calloc(1, sizeof(*conf));
    if (conf == NULL) {
        return NULL;
    }
    strcpy(conf->target_dir, "/tmp/");
    conf->bs = 1024 * 1024;

    return conf;
}


int config_defaults(struct rcp_config *conf, const char *buf, char *pos) {
    if (strcmp(buf, "remote_ip") == 0) {
        strncpy(conf->remote_ip, pos, sizeof(conf->remote_ip));
    } else if (strcmp(buf, "bs") == 0) {
        int val = atoi(pos);
        conf->bs = val;
    } else if (strcmp(buf, "target") == 0) {
//        int val = atoi(pos);
//        conf->beacon_int = val;
        strncpy(conf->target_dir, pos, sizeof(conf->remote_ip));
    }
    // more ...
    return 0;
}

struct rcp_config *config_read(const char *fname) {
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



