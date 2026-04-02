#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUF 4096

// Vérifie si fichier autorisé (.png, .jpg, .jpeg)
int allowed(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    return !strcmp(ext, ".png") ||
           !strcmp(ext, ".jpg") ||
           !strcmp(ext, ".jpeg");
}

// Retourne type MIME simple
const char* mime(const char *path) {
    if (strstr(path, ".png")) return "image/png";
    return "image/jpeg"; // jpg et jpeg
}

// Envoie 404 simple
void send_404(int client) {
    char *msg =
        "HTTP/1.1 404 Not Found\r\n\r\n404";
    send(client, msg, strlen(msg), 0);
}

// Envoie fichier au client
void send_file(int client, const char *path) {
    FILE *f = fopen(path, "rb"); // ouvrir fichier binaire
    if (!f) {
        send_404(client);
        return;
    }

    // Envoyer header HTTP
    char header[BUF];
    sprintf(header,
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n",
        mime(path)
    );
    send(client, header, strlen(header), 0);

    // Envoyer contenu fichier
    char buffer[BUF];
    int n;
    while ((n = fread(buffer, 1, BUF, f)) > 0) {
        send(client, buffer, n, 0);
    }

    fclose(f);
}

int main() {
    int server, client;
    struct sockaddr_in addr;
    char buffer[BUF];

    // Créer socket
    server = socket(AF_INET, SOCK_STREAM, 0);

    // Config serveur
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("http://127.0.0.1:%d\n", PORT);

    while (1) {
        // Attendre client
        client = accept(server, NULL, NULL);

        // Lire requête HTTP
        int r = read(client, buffer, BUF - 1);
        if (r <= 0) {
            close(client);
            continue;
        }
        buffer[r] = 0;

        // Extraire méthode + chemin
        char method[10], path[256];
        sscanf(buffer, "%s %s", method, path);

        // Accepter seulement GET
        if (strcmp(method, "GET") != 0) {
            close(client);
            continue;
        }

        // Sécurité basique
        if (strstr(path, "..")) {
            send_404(client);
            close(client);
            continue;
        }

        // Construire chemin réel
        char fullpath[512];
        sprintf(fullpath, "static%s", path);

        // Vérifier extension
        if (!allowed(fullpath)) {
            send_404(client);
        } else {
            send_file(client, fullpath);
        }

        close(client); // fermer connexion
    }

    return 0;
}
