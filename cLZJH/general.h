//
// Created by ngr02 on 2021/7/7.
//

#ifndef CLZJH_GENERAL_H
#define CLZJH_GENERAL_H

#define RESULT_SIZE 9000
/*!
 * @brief: the struct of the return value of a function in type of a pointer of unsigned char
 *         len: the length of what the pointer points to
 *         value: the pointer of the return value
 */
typedef struct struct_return
{
    unsigned int len;
    unsigned char *value;
} STRUCTRETURN;

/*!
 * @brief: the struct of the encode/decode result with the length of result and the array of result
 *         len: the length of the result
 *         result:
 *               Compress Mode: each the element of the array named result is a binary number(0 or 1)
 *               Decompress Mode: each of the element of the array named result is an eight bit number(0~255)
 */
typedef struct struct_result
{
    unsigned int len;
    unsigned char result[RESULT_SIZE];
} STRUCTRESULT;

/*!
 * @brief: read the file in binary mode
 * @param path: the path of the file to be read
 * @return: the struct of the return value with length of the length of file content and the content of the file
 */
STRUCTRETURN read_file_bin(char *path);

#endif //CLZJH_GENERAL_H
