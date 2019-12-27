/* This is the header file for rolo.c */

extern char *copystr(char *s);

extern char *timestring();

extern void user_interrupt();

extern int roloexit(int rval);

extern void save_to_disk();

extern void save_and_exit(int rval);

extern void user_eof();

extern char *rolo_emalloc(int size);

extern char *home_directory(char *name);

extern char *homedir(char *filename);

extern char *libdir(char *filename);

extern int rolo_only_to_read();

extern int locked_action();

extern int rolo_main (int argc, char *argv[]);

