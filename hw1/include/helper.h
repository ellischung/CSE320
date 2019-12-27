#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <sys/stat.h>

#include "transplant.h"
#include "const.h"

/**
 * Used to compare two strings because of the restriction of the string.h library.
 */
int check_flags(char *string1, char *string2);

/**
 * Used to return length of a string without the string.h library.
 */
int string_length(char *string);

/**
 * Used to clobber a file, if provided by path_buf
 */
int clobber_file();