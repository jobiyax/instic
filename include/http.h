#ifndef HTTP_H
#define HTTP_H

#include "config.h"

// Gère une requête client
void handle_request(int client_fd, server_config_t *config);

#endif
