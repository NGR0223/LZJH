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
    STRUCTRETURN struct_return = {0, NULL};

    // get the length in bytes of the file
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        perror("Fopen error");

        return struct_return;
    }
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);

    // read the content of the file
    struct_return.value = (unsigned char *) calloc(file_size + 1, sizeof(unsigned char));
    size_t result = fread(struct_return.value, 1, file_size, fp);
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
