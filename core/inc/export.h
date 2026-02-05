#ifndef HERMES_EXPORT_H
#define HERMES_EXPORT_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

/* CSV*/
int write_th_profile_csv(const char *path, const uint8_t reg[55], int is_p2);
int write_tvg_csv(const char *path, const uint8_t reg[55]);

/*JSON*/
int write_th_profile_json(const char *path, const uint8_t reg[55], int is_p2);
int write_tvg_json(const char *path, const uint8_t reg[55]);

#ifdef __cplusplus
}
#endif

#endif // HERMES_EXPORT_H