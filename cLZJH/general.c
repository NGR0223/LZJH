//
// Created by ngr02 on 2021/7/8.
//

#include "general.h"
#include <stdio.h>
#include <stdlib.h>

/*!
 * @brief: read the file in binary mode
 * @param path: the path of the file to be read
 * @return: the struct of the return value with length of the length of file content and the content of the file
 */
STRUCTRETURN read_file_bin(char *path)
{
    size_t file_size, result;
    FILE *fp = NULL;
    STRUCTRETURN struct_return;
    struct_return.len = 0;
    struct_return.value = NULL;

    // get the length in bytes of the file
    if ((fp = fopen(path, "rb")) == NULL)
    {
        perror("Fopen error");

        return struct_return;
    }
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // read the content of the file
    struct_return.value = (unsigned char *) calloc(file_size + 1, sizeof(unsigned char));
    result = fread(struct_return.value, 1, file_size, fp);
    if (result != file_size)
    {
        perror("Fread error");

        fclose(fp);

        return struct_return;
    }
    struct_return.len = file_size;
    struct_return.value[file_size] = '\0';
    fclose(fp);
    fp = NULL;

    return struct_return;
}
