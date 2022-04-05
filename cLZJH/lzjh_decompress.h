//
// Created by ngr02 on 2021/7/7.
//

#ifndef CLZJH_LZJH_DECOMPRESS_H
#define CLZJH_LZJH_DECOMPRESS_H

#include "general.h"

// the path of the file to be decompressed
#define MESSAGE_COMPRESSED_FILE_PATH "messageCompressed.txt"

/*!
 * @brief: the struct of string_collection, the member of it is defined according the file (V.44 Data Compression Procedures)
 */
typedef struct string_collection
{
    unsigned int last_char_pos;
    unsigned int codeword: 14;
    unsigned int string_length: 8;
} STRINGCOLLECTION;

/*!
 * @brief: the struct to simulator the class in python but the struct is not including the method just the data
 *      params: the param defined in the table 10 and table 11 in the Chapter 8 of the file (V.44 Data Compression Procedures)
 *      message_compressed: the pointer of the bit stream of the message compressed
 *      array_string_collection: the array of the struct of the string collection
 *      index_message_compressed: the index of the message_compressed
 *      len_message_compressed: the length of the message_compressed
 *      count_string_collection: the next available index of the array of the struct of the string collection
 *      flag_pre_code: the flag that records the type of the previous code
 *                     value --- type
 *                         0 --- ordinal
 *                         1 --- codeword
 *                         2 --- string-extension length
 *                         3 --- control code___FLUSH
 *                         4 --- control code___SETUP
 */
typedef struct dself
{
    unsigned int params[2][8];
    unsigned char *message_compressed;
    STRINGCOLLECTION array_string_collection[1024];
    unsigned int index_message_compressed: 16;
    unsigned int len_message_compressed: 16;
    unsigned int count_string_collection: 10;
    unsigned int flag_pre_code: 3;
    unsigned int flag_first_two_code: 3;
} DSELF;

/*!
 * @brief: the decompress function includes the decode function and the writing result to file function
 */
void decompress();

/*!
 * @brief: the decode function to recover the message_compressed
 * @param result_decompress: the struct to save the decode result
 */
void decode(STRUCTRESULT *result_decompress);

/*!
 * @brief: transfer the message_compressed to the bit stream
 * @param content: the content of the message compressed file
 * @return: the pointer of the unsigned char, but each elem it points to is binary number(0 or 1)
 */
unsigned char *set_content_to_bit_stream(STRUCTRETURN content);

/*!
 * @brief: handle an ordinal
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 */
void handle_ordinal(DSELF *self, STRUCTRESULT *result_decompress);

/*!
 * @brief:handle a codeword
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 * @param cur_code: the code to be handled
 */
void handle_codeword(DSELF *self, STRUCTRESULT *result_decompress, unsigned int cur_code);

/*!
 * @brief: handle a string-extension length
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 */
void handle_string_extension_length(DSELF *self, STRUCTRESULT *result_decompress);

/*!
 * @brief: handle a control code
 * @param self: the struct to save data
 * @param cur_code: the code to be handled
 */
void handle_control_code(DSELF *self, unsigned int cur_code);

/*!
 * @brief: create a new struct of string_collection
 * @param self: the struct to save data
 * @param last_char_pos: the last character position in the decode history
 * @param string_length: the length of the string represented by the string_collection
 */
void new_string_collection(DSELF *self, unsigned int last_char_pos, unsigned int string_length);

/*!
 * @brief: traverse the array of the string_collection by the codeword searched to get the string_collection whose codeword is equal to the codeword searched
 * @param self: the struct to save data
 * @param codeword_searched: the codeword to be searched
 * @return: the string_collection whose codeword is equal to the codeword searched
 */
STRINGCOLLECTION search_string_collection_by_codeword(DSELF *self, unsigned int codeword_searched);

/*!
 * @brief: transfer 1 to up to 12 bits to get the string-extension length in integer
 * @param self: the struct to save data
 * @return: the string-extension length
 */
unsigned int transfer_string_extension_length(DSELF *self);

/*!
 * @brief: save the decode result in the struct of result
 * @param result_decompress: the struct to save result
 * @param result_added: the result to be added
 */
void update_result_decompress(STRUCTRESULT *result_decompress, unsigned char result_added);

/*!
 * @brief: write the decompress result with the decompress result
 * @param result_decompress: the struct to save result
 */
void write_decompress_result_file(STRUCTRESULT result_decompress);

#endif //CLZJH_LZJH_DECOMPRESS_H
