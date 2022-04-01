//
// Created by ngr02 on 2021/6/20.
//

#ifndef CLZJH_LZJH_COMPRESS_H
#define CLZJH_LZJH_COMPRESS_H

#include "node.h"
#include "general.h"

#define MESSAGE_FILE_PATH "message.txt"

/*!
 * @brief: to simulate the class in python
 */
typedef struct cself
{
    ARRAYNODE array_node;
    unsigned int params[2][8];
    unsigned char *message;
    unsigned int len_message: 16;
    unsigned int index_message: 16;
    unsigned int count_codeword: 14;
    unsigned int flag_pre_code: 4;
} CSELF;

/*!
 * @brief:  the compress function includes the encode function and the writing result to file function
 */
void compress();

/*!
 * @brief: encode the file with message
 * @param result_compress: the struct to save the result of compress
 */
void encode(STRUCTRESULT *result_compress);

/*!
 * @brief: because the content of the file is hexadecimal numbers, the encode procedure should transform two hexadecimal digits to an unsigned char
 * @param content: the content of the file
 * @return: the point of the unsigned char string that has been transformed
 */
unsigned char *set_content_to_message(STRUCTRETURN content);

/*!
 * @brief: according to the flag of the type of the previous code and the flag of the type of the current code to determine the prefix
 * @param flag_pre_code: the flag of the type of the previous code
 * @param flag_cur_code: the flag of the type of the current code
 * @return: the prefix in unsigned char
 */
unsigned char get_prefix(unsigned int flag_pre_code, unsigned int flag_cur_code);

/*!
 * @brief: transform the prefix to bit stream
 * @param prefix: the prefix to be handled
 * @param struct_return: the struct to save the bit stream
 */
void handle_prefix(unsigned char prefix, STRUCTRETURN *struct_return);

/*!
 * @brief: handle a new character
 * @param cur_char: the new character
 * @param self: the struct to save data
 */
void new_character(unsigned char cur_char, CSELF *self);

/*!
 * @brief: set a code in unsigned int to bit stream
 * @param code: the code to be handled
 * @param len: the length of the bit stream, if the length of code in bit is less than the length that the function should set, the function should pad it with zero(0)
 * @param struct_return: the struct to save bit stream
 */
void handle_code(unsigned int code, unsigned char len, STRUCTRETURN *struct_return);

/*!
 * @brief: the string-extension procedure
 * @param self: the struct to save data
 * @param node_last_matched: the node that is the last node matched while matching the longest string
 * @return: the string-extension length
 */
unsigned int extend_string_segment(CSELF *self, NODE *node_last_matched);

/*!
 * @brief: transform the string-extension length to bit stream
 * @param string_extension_length: the string-extension length to be transformed
 * @param n7: the param that determines the length of the bit stream while the string-extension length is greater than 12
 * @param struct_return: the struct to save the bit stream
 */
void get_string_extension_length_subfields(unsigned char string_extension_length, unsigned int n7,
                                           STRUCTRETURN *struct_return);

/*!
 * @brief: update the result of compress with the code and the type of the code
 * @param result_compress: the struct to save result
 * @param self: the struct to save data
 * @param code: the code to be handled
 * @param type: the type of the code
 */
void update_result_compress(STRUCTRESULT *result_compress, CSELF *self, unsigned int code, unsigned int type);

/*!
 * @brief: write the file with the struct of result of compress
 * @param result_compress: the struct of result of compress
 */
void write_compress_result_file(STRUCTRESULT result_compress);

#endif //CLZJH_LZJH_COMPRESS_H
