#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/config.h"

// Charge et parse le fichier de configuration
int load_config(const char *filename, server_config_t *config)
{
    FILE *file = fopen(filename, "r");
    if (!file)
        return -1;

    char line[256];
    config->type_count = 0; // Initialise le nombre de types

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "PORT=", 5) == 0)
        {
            config->port = atoi(line + 5); // Lit le port
        }
        else if (strncmp(line, "ROOT=", 5) == 0)
        {
            sscanf(line + 5, "%s", config->root); // Lit le dossier racine
        }
        else if (strncmp(line, "ALLOW=", 6) == 0)
        {
            char ext[16], mime[64];

            sscanf(line + 6, "%15[^:]:%63s", ext, mime); // Extrait extension et mime

            strcpy(config->types[config->type_count].ext, ext);
            strcpy(config->types[config->type_count].mime, mime);

            config->type_count++; // Ajoute un type
        }
    }

    fclose(file);
    return 0;
}

// Vérifie autorisation
int is_allowed_type(server_config_t *config, const char *path)
{
    for (int i = 0; i < config->type_count; i++)
    {
        if (strstr(path, config->types[i].ext)) // Cherche extension dans le chemin
            return 1;
    }
    return 0;
}

// Retourne le MIME depuis config
const char *get_mime_type(server_config_t *config, const char *path)
{
    for (int i = 0; i < config->type_count; i++)
    {
        if (strstr(path, config->types[i].ext)) // Associe extension au mime
            return config->types[i].mime;
    }
    return "application/octet-stream"; // Type par défaut
}
