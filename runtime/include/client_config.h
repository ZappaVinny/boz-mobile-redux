#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reads <root>/client.ini (writing a default one if missing) into the BOZ_* settings. */
void client_config_load(const char *root);

/* A raw value from the [keys] section, or NULL when the file does not set it. */
const char *client_config_key_binding(const char *action);

/* Writes the commented default client.ini to path. */
bool client_config_write_default(const char *path);

#ifdef __cplusplus
}
#endif

#endif
