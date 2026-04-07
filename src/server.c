#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define PORT 9000
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

// Rendre un socket non bloquant (clé pour performance)
void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

// Détecte le type MIME pour le navigateur
const char *get_content_type(const char *path)
{
    if (strstr(path, ".png"))
        return "image/png";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg"))
        return "image/jpeg";
    return "application/octet-stream";
}

// Réponse simple 404
void send_404(int client_fd)
{
    char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";

    send(client_fd, response, strlen(response), 0);
}

// Traite une requête HTTP
void handle_request(int client_fd)
{
    char buffer[BUFFER_SIZE] = {0};
    read(client_fd, buffer, BUFFER_SIZE); // lire requête

    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path); // extraire GET /file

    // Accepter uniquement GET
    if (strcmp(method, "GET") != 0)
    {
        close(client_fd);
        return;
    }

    // Bloquer accès système
    if (strstr(path, ".."))
    {
        send_404(client_fd);
        close(client_fd);
        return;
    }

    char file_path[512];

    // Route par défaut
    if (strcmp(path, "/") == 0)
        strcpy(file_path, "./static/instic_logo.png");
    else
        snprintf(file_path, sizeof(file_path), "./static%s", path);

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0)
    {
        send_404(client_fd);
        close(client_fd);
        return;
    }

    struct stat st;
    fstat(file_fd, &st);

    const char *content_type = get_content_type(file_path);

    // Header HTTP
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n\r\n",
             st.st_size, content_type);

    send(client_fd, header, strlen(header), 0);

    // Envoi rapide du fichier (zero-copy)
    sendfile(client_fd, file_fd, NULL, st.st_size);

    close(file_fd);
    close(client_fd);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("🚀 Serveur fonctionnant sur le port:%d\n", PORT);

    while (1)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            // Nouvelle connexion
            if (fd == server_fd)
            {
                int client_fd = accept(server_fd, NULL, NULL);
                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            // Requête client prête
            else
            {
                handle_request(fd);
            }
        }
    }

    close(server_fd);
    return 0;
}
