#ifndef CALYNDA_METADATA_H
#define CALYNDA_METADATA_H

#define CALYNDA_CLI_NAME "calynda"
#define CALYNDA_CLI_VERSION "1.0.0-alpha.2"
#define CALYNDA_CLI_VERSION_LINE CALYNDA_CLI_NAME " " CALYNDA_CLI_VERSION

static inline const char *calynda_cli_name(void) {
    return CALYNDA_CLI_NAME;
}

static inline const char *calynda_cli_version(void) {
    return CALYNDA_CLI_VERSION;
}

static inline const char *calynda_cli_version_line(void) {
    return CALYNDA_CLI_VERSION_LINE;
}

#endif
