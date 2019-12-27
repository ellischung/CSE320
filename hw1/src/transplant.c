#include "const.h"
#include "transplant.h"
#include "helper.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/*
 * Global variables that will help with programming
 */
uint32_t depth_counter = 0;
uint64_t head_size = HEADER_SIZE;
uint64_t dirent_size = 28;
uint32_t type_info;
uint64_t size_info;
uint32_t current_depth;
uint64_t record_size;
uint32_t permission_bit_0;
uint32_t permission_bit_1;
uint32_t permission_bit_2;
uint32_t permission_bit_3;
uint64_t entry_size;
int entry_length;

/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
	return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
	return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
	return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
	return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
	return "DIRECTORY_ENTRY";
    case FILE_DATA:
	return "FILE_DATA";
    default:
	return "UNKNOWN";
    }
}

/**
 * Used to return length of a string without the string.h library.
 */
int string_length(char *string) {
    int length = 0;
    while(*string != 0) {
        string++;
        length++;
    }
    return length;
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    // if condition for invalid argument string
    if(string_length(name) + 1 > PATH_MAX) {
        return -1;
    }

    // for loop to add each char in name to path_buf, then add the null byte
    int i;
    for(i = 0; i < string_length(name); i++) {
        *(path_buf + i) = *(name + i);
    }

    // set path_length to length of name WITHOUT the null byte
    path_length = string_length(path_buf);

    // success, so return 0
    return 0;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    // if condition for invalid argument string
    if(string_length(name) + 1 > PATH_MAX) {
        return -1;
    }

    // second if condition for invalid argument string
    int i;
    for(i = 0; i < string_length(name); i++) {
        if(*(name + i) == '/') {
            return -1;
        }
    }

    // append a "/" to the existing path and then the argument string
    *(path_buf + path_length) = '/';
    int j = 0;
    for(i = path_length + 1; i < path_length + string_length(name) + 1; i++) {
        *(path_buf + i) = *(name + j);
        j++;
    }

    // update path_length
    path_length = string_length(path_buf);

    // success, so return 0
    return 0;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    // if conditional to check if path_buf is empty
    if(path_length == 0) {
        return -1;
    }

    //check if there is an occurence of '/' by creating empty_bit
    int i;
    int empty_bit = 1;
    for(i = 0; i < path_length; i++) {
        if(*(path_buf + i) == '/') {
            empty_bit = 0;
        }
    }

    // non-empty path_buf pop, return 0 after
    if(empty_bit == 0) {
        i = path_length;
        while(*(path_buf + i) != '/') {
            *(path_buf + i) = '\0';
            i--;
        }
        *(path_buf + i) = '\0';

        // update path_length
        path_length = string_length(path_buf);

        return 0;
    }

    // empty path_buf pop, return 0 after
    if(empty_bit == 1) {
        for(i = path_length; i > 0; i--) {
            *(path_buf + i) = '\0';
        }
        *path_buf = '\0';

        // update path_length
        path_length = string_length(path_buf);

        return 0;
    }

    // EOF error
    return -1;
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    // necessary variables
    int i;

    // if depth = 0, there is an error
    if (depth == 0) {
        return -1;
    }
    else {
        // push name_buf onto path_bufband then make the directory for it
        path_push(name_buf);
        mkdir(path_buf, 0700);
    }

    // set permission bits
    chmod(path_buf, (permission_bit_0 << 27) & 0777);
    chmod(path_buf, (permission_bit_1 << 18) & 0777);
    chmod(path_buf, (permission_bit_2 << 9) & 0777);
    chmod(path_buf, (permission_bit_3) & 0777);

    // check if depth is +1 more than current depth later but not now.. (to ease up things)
    for(i = 0; i < 4; i++) {
        getchar();
    }

    // record size check
    record_size = 0;
    for(i = 0; i < 8; i++) {
        record_size = record_size + getchar();
    }

    // error in record size
    if(record_size != 16) {
        return -1;
    }

    // success, so return 0
    return 0;
}

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth) {
    // necessary variables
    int i;
    int j = 0;
    uint32_t depth_check = 0;
    uint64_t file_check = 0;

    // file depth check
    for(i = 0; i < 4; i++) {
        depth_check = depth_check + getchar();
    }
    if(depth_check != depth) {
        return -1;
    }

    // new depth
    current_depth = depth_check;

    // record header size check
    for(i = 0; i < 8; i++) {
        file_check = file_check + getchar();
    }
    if(file_check - HEADER_SIZE != entry_size) {
        return -1;
    }

    // push name_buf onto path_buf
    path_push(name_buf);

    // create the file with path_buf
    FILE *f = fopen(path_buf, "w");

    // open file fail
    if (f == NULL) {
        return -1;
    }

    // set permission bits
    chmod(path_buf, (permission_bit_0 << 27) & 0777);
    chmod(path_buf, (permission_bit_1 << 18) & 0777);
    chmod(path_buf, (permission_bit_2 << 9) & 0777);
    chmod(path_buf, (permission_bit_3) & 0777);

    // write each next byte onto file f
    for(i = 0; i < entry_size; i++) {
        fputc(getchar(), f);
    }

    // pop off the filename
    path_pop();

    // success, so return 0
    return 0;
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth) {
    // necessary variables
    DIR *dir;
    struct dirent *de;
    int i;

    // start of new directory
    depth_counter = depth;
    putchar(MAGIC0);
    putchar(MAGIC1);
    putchar(MAGIC2);
    putchar(START_OF_DIRECTORY);
    for(i = 3; i >= 0; i--) {
        putchar((depth_counter >> i*8) & 0xFF);
    }
    for(i = 7; i >= 0; i--) {
        putchar((head_size >> i*8) & 0xFF);
    }

    // error if failure to open directory
    if((dir = opendir(path_buf)) == NULL) {
        return -1;
    }

    // error case for readdir

    // iterate through directory
    while((de = readdir(dir)) != NULL) {
        // if directory is "." or "..", continue loop (skip)
        if(check_flags(de->d_name, ".") || check_flags(de->d_name, "..")) {
            continue;
        }

        // check if we're looking at a file or directory
        struct stat stat_buf;
        path_push(de->d_name);
        int stat_check = stat(path_buf, &stat_buf);

        // error if failure in stat call
        if(stat_check != 0) {
            return -1;
        }

        // check to serialize file
        if(S_ISREG(stat_buf.st_mode)) {
            // set metadata for DIRECTORY_ENTRY record
            type_info = stat_buf.st_mode;
            size_info = stat_buf.st_size;

            // DIRECTORY_ENTRY record
            putchar(MAGIC0);
            putchar(MAGIC1);
            putchar(MAGIC2);
            putchar(DIRECTORY_ENTRY);
            for(i = 3; i >= 0; i--) {
                putchar((depth_counter >> i*8) & 0xFF);
            }
            for(i = 7; i >= 0; i--) {
                putchar(((dirent_size + string_length(de->d_name)) >> i*8) & 0xFF);
            }
            for(i = 3; i >= 0; i--) {
                putchar((type_info >> i*8) & 0xFF);
            }
            for(i = 7; i >= 0; i--) {
                putchar((size_info >> i*8) & 0xFF);
            }
            for(i = 0; i < string_length(de->d_name); i++) {
                putchar(*(de->d_name + i));
            }

            // serialize the file
            serialize_file(depth_counter, stat_buf.st_size + HEADER_SIZE);

            // pop the cwd
            path_pop();
        }

        // check to serialize directory
        if(S_ISDIR(stat_buf.st_mode)) {
            // set metadata for DIRECTORY_ENTRY record
            type_info = stat_buf.st_mode;
            size_info = stat_buf.st_size;

            // DIRECTORY ENTRY record
            putchar(MAGIC0);
            putchar(MAGIC1);
            putchar(MAGIC2);
            putchar(DIRECTORY_ENTRY);
            for(i = 3; i >= 0; i--) {
                putchar((depth_counter >> i*8) & 0xFF);
            }
            for(i = 7; i >= 0; i--) {
                putchar(((dirent_size + string_length(de->d_name)) >> i*8) & 0xFF);
            }
            for(i = 3; i >= 0; i--) {
                putchar((type_info >> i*8) & 0xFF);
            }
            for(i = 7; i >= 0; i--) {
                putchar((size_info >> i*8) & 0xFF);
            }
            for(i = 0; i < string_length(de->d_name); i++) {
                putchar(*(de->d_name + i));
            }

            // serialize the directory
            depth_counter++;
            serialize_directory(depth_counter);

            // pop the cwd
            path_pop();
        }
    }

    // close directory
    int close_dir = closedir(dir);

    // close directory fail
    if(close_dir != 0) {
        return -1;
    }

    // end of directory
    depth_counter = depth;
    putchar(MAGIC0);
    putchar(MAGIC1);
    putchar(MAGIC2);
    putchar(END_OF_DIRECTORY);
    for(i = 3; i >= 0; i--) {
        putchar((depth_counter >> i*8) & 0xFF);
    }
    for(i = 7; i >= 0; i--) {
        putchar((head_size >> i*8) & 0xFF);
    }

    // success, so return 0
    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {
    // serialize the FILE_DATA record to stdout

    // start with magic bytes
    putchar(MAGIC0);
    putchar(MAGIC1);
    putchar(MAGIC2);

    // this is a type FILE_DATA
    putchar(FILE_DATA);

    // depth (4 bytes)
    int i;
    for(i = 3; i >= 0; i--) {
        putchar((depth >> i*8) & 0xFF);
    }

    // open the file that we are serializing and extract data
    FILE *f = fopen(path_buf, "r");

    // open file fail
    if (f == NULL) {
        return -1;
    }

    // record header size
    for(i = 7; i >= 0; i--) {
        putchar((size >> i*8) & 0xFF);
    }

    // reset file pointer
    fseek(f, 0, SEEK_SET);

    // grab each char in file again and print to stdout
    char file_char;
    while((file_char = fgetc(f)) != EOF) {
        putchar(file_char);
    }

    // close file
    int file_close = fclose(f);

    // close file fail
    if(file_close != 0) {
        return -1;
    }

    // success, so return 0
    return 0;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {
    // necessary variables
    int i;
    int j = 0;
    int file_length = 0;

    // start of transmission
    putchar(MAGIC0);
    putchar(MAGIC1);
    putchar(MAGIC2);
    putchar(START_OF_TRANSMISSION);
    for(i = 3; i >= 0; i--) {
        putchar((depth_counter >> i*8) & 0xFF);
    }
    for(i = 7; i >= 0; i--) {
        putchar((head_size >> i*8) & 0xFF);
    }

    // check if we're looking at file or directory
    struct stat stat_buf;
    stat(path_buf, &stat_buf);
    int stat_check = stat(path_buf, &stat_buf);

    // error if failure in stat call
    if(stat_check != 0) {
        return -1;
    }

    if(S_ISREG(stat_buf.st_mode)) {
        // we need a directory entry for this file
        // set metadata for DIRECTORY_ENTRY record
        type_info = stat_buf.st_mode;
        size_info = stat_buf.st_size;

        // get the name of the file and store it in name_buf
        i = path_length - 1;
        while(i > 0) {
            if(*(path_buf + i) == '/') {
                break;
            }
            file_length++;
            i--;
        }
        for(i = path_length - file_length; i < path_length; i++) {
            *(name_buf + j) = *(path_buf + i);
            j++;
        }

        // start the directory entry
        putchar(MAGIC0);
        putchar(MAGIC1);
        putchar(MAGIC2);
        putchar(DIRECTORY_ENTRY);
        for(i = 3; i >= 0; i--) {
            putchar((depth_counter >> i*8) & 0xFF);
        }
        for(i = 7; i >= 0; i--) {
            putchar(((dirent_size + string_length(name_buf)) >> i*8) & 0xFF);
        }
        for(i = 3; i >= 0; i--) {
            putchar((type_info >> i*8) & 0xFF);
        }
        for(i = 7; i >= 0; i--) {
            putchar((size_info >> i*8) & 0xFF);
        }
        for(i = 0; i < string_length(name_buf); i++) {
            putchar(*(name_buf + i));
        }

        // serialize file
        serialize_file(depth_counter, stat_buf.st_size + HEADER_SIZE);
    }

    if(S_ISDIR(stat_buf.st_mode)) {
        // serialize directory
        depth_counter++;
        serialize_directory(depth_counter);
    }

    // end of transmission
    depth_counter = 0;
    putchar(MAGIC0);
    putchar(MAGIC1);
    putchar(MAGIC2);
    putchar(END_OF_TRANSMISSION);
    for(i = 3; i >= 0; i--) {
        putchar((depth_counter >> i*8) & 0xFF);
    }
    for(i = 7; i >= 0; i--) {
        putchar((head_size >> i*8) & 0xFF);
    }

    // success, so return 0
    return 0;
}

/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize() {
    // necessary variables
    struct stat stat_buf;
    int i;
    int depth_checker;
    unsigned char mag0;
    unsigned char mag1;
    unsigned char mag2;
    unsigned char record_type;

    // check if directory already exists, if it doesn't, make one
    if (stat(path_buf, &stat_buf) == -1) {
        mkdir(path_buf, 0700);

        // read each byte in stdin
        mag0 = getchar();
        mag1 = getchar();
        mag2 = getchar();
        record_type = getchar();

        // check if transmission has begun
        if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == START_OF_TRANSMISSION) {
            // begin deserializing contents
            // seek to depth and extract it
            current_depth = 0;
            for(i = 0; i < 4; i++) {
                current_depth = current_depth + getchar();
            }

            // error in depth size
            if(current_depth != 0) {
                return -1;
            }

            // seek to record size and extract it
            record_size = 0;
            for(i = 0; i < 8; i++) {
                record_size = record_size + getchar();
            }

            // error in record size
            if(record_size != 16) {
                return -1;
            }

            // next record header checkpoint
            mag0 = getchar();
            mag1 = getchar();
            mag2 = getchar();
            record_type = getchar();

            // check for directory
            if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == START_OF_DIRECTORY) {
                current_depth++;

                // seek to depth and extract it
                depth_checker = 0;
                for(i = 0; i < 4; i++) {
                    depth_checker = depth_checker + getchar();
                }

                // error in depth size
                if(depth_checker != current_depth) {
                    return -1;
                }

                // extract size of record
                record_size = 0;
                for(i = 0; i < 8; i++) {
                    record_size = record_size + getchar();
                }

                // error in record size
                if(record_size != 16) {
                    return -1;
                }

                // after checks, we seek onto next record header
                mag0 = getchar();
                mag1 = getchar();
                mag2 = getchar();
                record_type = getchar();

                // we now procced to check if we can enter the directory entry LOOP

            }

            // directory entry LOOP begins here
            while((mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == DIRECTORY_ENTRY) || current_depth != 0) {
                // if we are finished with all files/directories, start locating ends
                if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == END_OF_DIRECTORY) {
                    // pop name_buf from path_buf
                    path_pop();

                    // check the record header's depth
                    depth_checker = 0;
                    for(i = 0; i < 4; i++) {
                        depth_checker = depth_checker + getchar();
                    }

                    // error in depth after successive decrementing of depth
                    if(depth_checker != current_depth) {
                        return -1;
                    }

                    // decrement depth after the check
                    current_depth--;

                    // record size (must be 16)
                    record_size = 0;
                    for(i = 0; i < 8; i++) {
                        record_size = record_size + getchar();
                    }

                    // error in record size
                    if(record_size != 16) {
                        return -1;
                    }

                    // seek to next record header
                    mag0 = getchar();
                    mag1 = getchar();
                    mag2 = getchar();
                    record_type = getchar();

                    // do not execute below, continue loop
                    continue;

                }


                // THIS REGION IS FOR IF WE HAVE NOT REACHED THE END


                // extract depth
                depth_checker = 0;
                for(i = 0; i < 4; i++) {
                    depth_checker = depth_checker + getchar();
                }

                // error in depth size (two test cases)
                if(current_depth == 0) {
                    if(depth_checker != 0) {
                        return -1;
                    }
                }
                else {
                    if(depth_checker != current_depth) {
                        return -1;
                    }
                }

                // extract size of record and then do arithmetic to get entry_length
                record_size = 0;
                for(i = 0; i < 8; i++) {
                    record_size = record_size + getchar();
                }

                entry_length = record_size - 28;

                // we are now at the metadata

                // extract permission bits
                permission_bit_0 = 0;
                permission_bit_1 = 0;
                permission_bit_2 = 0;
                permission_bit_3 = 0;
                permission_bit_0 = permission_bit_0 + getchar();
                permission_bit_1 = permission_bit_1 + getchar();
                permission_bit_2 = permission_bit_2 + getchar();
                permission_bit_3 = permission_bit_3 + getchar();

                // extract size of entry
                entry_size = 0;
                for(i = 0; i < 8; i++) {
                    entry_size = entry_size + getchar();
                }

                // clear out the name_buf FIRST
                for(i = string_length(name_buf); i >= 0; i--) {
                    *(name_buf + i) = '\0';
                }

                // store the filename into name_buf
                for(i = 0; i < entry_length; i++) {
                    *(name_buf + i) = getchar();
                }

                // now check for file/directory, error if anything else
                mag0 = getchar();
                mag1 = getchar();
                mag2 = getchar();
                record_type = getchar();

                if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == FILE_DATA) {
                    // call deserialize_file
                    deserialize_file(current_depth);

                    // if depth is not 0, seek to next record header
                    if(current_depth != 0) {
                        mag0 = getchar();
                        mag1 = getchar();
                        mag2 = getchar();
                        record_type = getchar();
                    }

                }
                else if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == START_OF_DIRECTORY) {
                    // increment depth and then call deserialize_directory
                    current_depth++;
                    deserialize_directory(current_depth);

                    // if depth is not 0, seek to next record header
                    if(current_depth != 0) {
                        mag0 = getchar();
                        mag1 = getchar();
                        mag2 = getchar();
                        record_type = getchar();
                    }

                }
                else {
                    // error in record header
                    return -1;
                }
            }

            // no need to check for anything if we have NOT started a directory

        }
        else {
            // start of transmission was not located
            return -1;
        }
    }
    else {
        // directory already exists, abort
        return -1;
    }

    // check for end of transmission now
    if(mag0 == MAGIC0 && mag1 == MAGIC1 && mag2 == MAGIC2 && record_type == END_OF_TRANSMISSION) {
        // seek to depth and extract it
        current_depth = 0;
        for(i = 0; i < 4; i++) {
            current_depth = current_depth + getchar();
        }

        // error in depth size
        if(current_depth != 0) {
            return -1;
        }

        // extract size of record
        record_size = 0;
        for(i = 0; i < 8; i++) {
            record_size = record_size + getchar();
        }

        // error in record size
        if(record_size != 16) {
            return -1;
        }

        //success, so return 0
        return 0;
    }

    // EOF
    return -1;
}

/**
 * Used to compare two strings because of the restriction of the string.h library.
 */
int check_flags(char *string1, char *string2) {
    // set counter variable
    int c = 0;

    // use a while loop to check contents of each char
    while(*(string1 + c) == *(string2 + c)) {
        if(*(string1 + c) == '\0' || *(string2 + c) == '\0') {
            break;
        }
        c++;
    }

    // conditional to return whether or not flags match
    if(*(string1 + c) == '\0' && *(string2 + c) == '\0') {
        return -1;
    }
    else {
        return 0;
    }
}

/**
 * Used to clobber a file, if provided by path_buf
 */
int clobber_file() {
    // check if we're looking at file
    struct stat stat_buf;
    stat(path_buf, &stat_buf);
    int stat_check = stat(path_buf, &stat_buf);

    // error if failure in stat call
    if(stat_check != 0) {
        return -1;
    }

    // if it's a file, open it and clear it out
    if(S_ISREG(stat_buf.st_mode)) {
        // open file (automatically clears it with "w" flag)
        FILE *f = fopen(path_buf, "w");

        // open file fail
        if (f == NULL) {
            return -1;
        }

        // close file
        int file_close = fclose(f);

        // close file fail
        if(file_close != 0) {
            return -1;
        }

        // success, so return 0
        return 0;
    }
    else {
        // not a file, we can exit with success still
        return 0;
    }

    // EOF
    return -1;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    // start a counter
    int i;
    global_options = 0;
    global_options = global_options >> 4;

    // check num of arguments
    if(argc > 1) {
        // check each argument starting with the first
        for(i = 1; i < argc; i++) {
            // all checks for -h flag
            if(check_flags(*(argv + i), "-h")) {
                if(i == 1) {
                    // perform (-h) operation
                    global_options = global_options | 0x1;
                    return 0;
                }
                else {
                    //error in -h placement
                    return -1;
                }
            }

            // all checks for -s flag
            else if(check_flags(*(argv + i), "-s")) {
                if(i == 1) {
                    continue;
                }
                else {
                    //error in -s placement
                    return -1;
                }
            }

            // all checks for -d flag
            else if(check_flags(*(argv + i), "-d")) {
                if(i == 1) {
                    continue;
                }
                else {
                    //error in -d placement
                    return -1;
                }
            }

            // all checks for -c flag
            else if(check_flags(*(argv + i), "-c")) {
                if(i == 1) {
                    continue;
                }
                else if(i == 2 && check_flags(*(argv + 1), "-d")) {
                    continue;
                }
                else if(i == 3 && !check_flags(*(argv + i), "-h") && !check_flags(*(argv + i), "-s") && !check_flags(*(argv + i), "-d") && !check_flags(*(argv + i), "-p")) {
                    continue;
                }
                else if(i == 4 && !check_flags(*(argv + i), "-h") && !check_flags(*(argv + i), "-s") && !check_flags(*(argv + i), "-d") && !check_flags(*(argv + i), "-p")) {
                    continue;
                }
                else {
                    //error in -c placement
                    return -1;
                }
            }

            // all checks for -p DIR flag
            else if(check_flags(*(argv + i), "-p")) {
                if((i == 1 && argc > 2) || (i == 2 && argc > 3) || (i == 3 && argc > 4)) {
                    i++;
                    if(!check_flags(*(argv + i), "-h") && !check_flags(*(argv + i), "-s") && !check_flags(*(argv + i), "-d") && !check_flags(*(argv + i), "-p")) {
                        continue;
                    }
                    else {
                        //error in -p DIR placement
                        return -1;
                    }
                }
                else {
                    //error in -p DIR placement
                    return -1;
                }
            }

            // NO recognizable argument at this point
            else {
                // random arguments that aren't flags
                return -1;
            }
        }

        // for loop halts and we are at valid arguments

        // create another for loop that checks which operation to
        // perform (-s, -d, -c) now that the arguments are completely valid.
        for(i = 1; i < argc; i++) {
            if(check_flags(*(argv + i), "-s")) {
                global_options = global_options | 0x2;
                continue;
            }
            else if(check_flags(*(argv + i), "-d")) {
                global_options = global_options | 0x4;
                continue;
            }
            else if(check_flags(*(argv + i), "-c")) {
                global_options = global_options | 0x8;
                continue;
            }
            else if(check_flags(*(argv + i), "-p")) {
                path_init(*(argv + i + 1));
            }
        }

        // no parameter given for -p
        i = argc - 1;
        if(check_flags(*(argv + i), "-c") || check_flags(*(argv + i), "-p")) {
            path_init("./");
        }

        //functions performed successfully at this point
        return 0;
    }
    else {
        // Only 1 argument entered (bin/transplant)
        return -1;
    }

    // EOF
    return -1;
}
