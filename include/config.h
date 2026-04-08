#ifndef CONFIG_H
#define CONFIG_H

#define MAX_TYPES 32

typedef struct
{
    char ext[16];
    char mime[64];
} mime_entry_t;

typedef struct
{
    int port;
    char root[256];

    mime_entry_t types[MAX_TYPES];
    int type_count;

} server_config_t;

// Charge la configuration depuis un fichier
int load_config(const char *filename, server_config_t *config);

// Vérifie si le type de fichier est autorisé
int is_allowed_type(server_config_t *config, const char *path);

// Retourne le type MIME d'un fichier
const char *get_mime_type(server_config_t *config, const char *path);

#endif
