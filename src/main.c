#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "../include/config.h"
#include "../include/http.h"
#include "../include/utils.h"

#define MAX_EVENTS 1024

int main()
{
    server_config_t config;

    if (load_config("config.conf", &config) < 0) // Charge la configuration
    {
        printf("Erreur chargement config\n");
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // Création du socket serveur

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)); // Associe socket à une adresse
    listen(server_fd, SOMAXCONN);                            // Écoute des connexions

    set_nonblocking(server_fd); // Socket non bloquant

    int epoll_fd = epoll_create1(0); // Crée instance epoll

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev); // Ajoute serveur à epoll

    printf("🚀 Serveur instic sur port: %d\n", config.port);

    while (1)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // Attend événements

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd) // Nouvelle connexion
            {
                int client_fd = accept(server_fd, NULL, NULL);
                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev); // Ajoute client à epoll
            }
            else
            {
                handle_request(fd, &config); // Traite requête client
            }
        }
    }

    close(server_fd);
    return 0;
}
