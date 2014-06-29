/* getopt.h */

#ifndef UNIX      /* avoid conflict with stdlib.h */

#ifdef __cplusplus
extern "C" {
#endif

int getopt(int argc, char **argv, char *opts);
extern char *optarg;
extern int optind;

#ifdef __cplusplus
}
#endif

#endif