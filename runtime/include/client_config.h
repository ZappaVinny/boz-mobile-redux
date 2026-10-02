#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

/* Reads <root>/client.ini (writing a default one if missing) into the BOZ_* settings. */
void client_config_load(const char *root);

/* A raw value from the [keys] section, or NULL when the file does not set it. */
const char *client_config_key_binding(const char *action);

#endif
