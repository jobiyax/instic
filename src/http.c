#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "../include/http.h"
#include "../include/config.h"

#define BUFFER_SIZE 4096

// Envoie une réponse 404
void send_404(int client_fd)
{
    char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n\r\n";

    send(client_fd, response, strlen(response), 0);
}

// Envoie une réponse 403
void send_403(int client_fd)
{
    char *response =
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Length: 0\r\n\r\n";

    send(client_fd, response, strlen(response), 0);
}

// Gère une requête HTTP client
void handle_request(int client_fd, server_config_t *config)
{
    char buffer[BUFFER_SIZE] = {0};

    int r = read(client_fd, buffer, BUFFER_SIZE);
    if (r <= 0) // Erreur ou connexion fermée
    {
        close(client_fd);
        return;
    }

    char method[16], path[256];
    sscanf(buffer, "%15s %255s", method, path); // Extrait méthode et chemin

    printf("[REQ] %s %s\n", method, path);

    if (strcmp(method, "GET") != 0) // Accepte seulement GET
    {
        close(client_fd);
        return;
    }

    if (strstr(path, "..")) // Sécurité contre path traversal
    {
        send_403(client_fd);
        close(client_fd);
        return;
    }

    char file_path[512];

    if (strcmp(path, "/") == 0)
    {
        snprintf(file_path, sizeof(file_path),
                 "%s/instic_logo.png", config->root);
    }
    else
    {
        snprintf(file_path, sizeof(file_path),
                 "%s%s", config->root, path);
    }

    printf("[FILE] %s\n", file_path);

    if (!is_allowed_type(config, file_path)) // Vérifie autorisation du type
    {
        send_403(client_fd);
        close(client_fd);
        return;
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) // Fichier introuvable
    {
        send_404(client_fd);
        close(client_fd);
        return;
    }

    struct stat st;
    if (fstat(file_fd, &st) < 0) // Récupère infos fichier
    {
        close(file_fd);
        close(client_fd);
        return;
    }

    const char *type = get_mime_type(config, file_path); // Récupère type MIME

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: %s\r\n"
             "Connection: close\r\n\r\n",
             st.st_size, type);

    send(client_fd, header, strlen(header), 0);
    sendfile(client_fd, file_fd, NULL, st.st_size); // Envoie le fichier

    close(file_fd);
    close(client_fd);
}
