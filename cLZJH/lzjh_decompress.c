//
// Created by ngr02 on 2021/7/7.
//

#include "lzjh_decompress.h"
#include <stdio.h>
#include <stdlib.h>

/*!
 * @brief: the decompress function includes the decode function and the writing result to file function
 */
void decompress()
{
    STRUCTRESULT result_decompress = {0, {0}};

    // decode
    decode(&result_decompress);

    // write the file with the result
    write_decompress_result_file(result_decompress);
    puts("Decompressed successfully");
}

/*!
 * @brief: transfer a bit string to an unsigned integer decimal number
 * @param self: the struct to save data
 * @param length: the length of the bit string
 * @param last_bit_pos: the last position of the bit string in the message_compressed
 * @return: the unsigned integer decimal number of the bit string
 */
static unsigned int get_code(DSELF *self, unsigned char length, unsigned int last_bit_pos)
{
    unsigned int cur_code = 0;

    for (int i = 0; i < length; ++i)
    {
        cur_code += self->message_compressed[(unsigned int) last_bit_pos - 1 - i] << i;
    }

    return cur_code;
}

/*!
 * @brief: the decode function to recover the message_compressed
 * @param result_decompress: the struct to save the decode result
 */
void decode(STRUCTRESULT *result_decompress)
{
    // get message compressed
    STRUCTRETURN struct_return = read_file_bin(MESSAGE_COMPRESSED_FILE_PATH);

    //Init data structure and other variables
    DSELF self = {{{10, 1024, 8, 256, 4, 0, 255, 3072},
                   {4, 6, 64, 0, 7}}, NULL, {0}, struct_return.len * 4 - 1, struct_return.len * 4, 0, 4, 7, 7};
    self.message_compressed = set_content_to_bit_stream(struct_return);
    free(struct_return.value);
    struct_return.value = NULL;

    unsigned int cur_code = 0;
    while ((unsigned int) self.index_message_compressed < self.len_message_compressed)
    {
        // the first bit must be a prefix, according to the first bit to determine the type of the following code
        if (self.index_message_compressed == (unsigned int) self.len_message_compressed - 1)
        {
            // the first bit is zero(0), so the code is an ordinal
            if (self.message_compressed[self.index_message_compressed] == 0)
            {
                handle_ordinal(&self, result_decompress);
                self.flag_pre_code = 0;
            }
            else
            {
                // the first bit is one(1), so the code is a code(codeword or control code)
                cur_code = get_code(&self, self.params[1][1], self.index_message_compressed);
                self.index_message_compressed -= self.params[1][1] + 1;
                //  if the value of the code is less than 4(0~3), the code is a control code; otherwise, the code is a codeword
                if (cur_code < 4)
                {
                    handle_control_code(&self, cur_code);
                    self.flag_first_two_code = self.flag_pre_code;
                    self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                }
                else
                {
                    handle_codeword(&self, result_decompress, cur_code);
                    self.flag_pre_code = 1;
                }
            }
        }
        else
        {
            // if the first code has been handled, the remaining codes depend on the flag of previous code to decide how to handle them
            // see the Chapter 6.6.3(P15) of the file(V.44 Data Compression Procedures) for specific rules
            // the previous code is a codeword
            if ((unsigned int) self.flag_pre_code == 1)
            {
                // the current code is a codeword or a control code
                if (self.message_compressed[self.index_message_compressed] == 1)
                {
                    cur_code = get_code(&self, self.params[1][1], self.index_message_compressed);
                    self.index_message_compressed -= self.params[1][1] + 1;
                    if (cur_code < 4)
                    {
                        handle_control_code(&self, cur_code);
                        self.flag_first_two_code = self.flag_pre_code;
                        self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                    }
                    else
                    {
                        handle_codeword(&self, result_decompress, cur_code);
                        self.flag_pre_code = 1;
                    }
                }
                else
                {
                    self.index_message_compressed--;
                    // the current code is an ordinal or a string-extension length
                    if (self.message_compressed[self.index_message_compressed] == 1)
                    {
                        handle_string_extension_length(&self, result_decompress);
                        self.flag_pre_code = 2;
                    }
                    else
                    {
                        handle_ordinal(&self, result_decompress);
                        self.flag_pre_code = 0;
                    }
                }
            }
            else // the previous code is other codes
            {
                // the current code is a codeword or a control code
                if (self.message_compressed[self.index_message_compressed] == 1)
                {
                    cur_code = get_code(&self, self.params[1][1], self.index_message_compressed);
                    self.index_message_compressed -= self.params[1][1] + 1;
                    if (cur_code < 4)
                    {
                        handle_control_code(&self, cur_code);
                        self.flag_first_two_code = self.flag_pre_code;
                        self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                    }
                    else
                    {
                        handle_codeword(&self, result_decompress, cur_code);
                        self.flag_pre_code = 1;
                    }
                }
                else // the current code is an ordinal
                {
                    handle_ordinal(&self, result_decompress);
                    self.flag_pre_code = 0;
                }
            }
        }
    }

    free(self.message_compressed);
    self.message_compressed = NULL;
}

/*!
 * @brief: transfer the message_compressed to the bit stream
 * @param content: the content of the message compressed file
 * @return: the pointer of the unsigned char, but each elem it points to is binary number(0 or 1)
 */
unsigned char *set_content_to_bit_stream(STRUCTRETURN content)
{
    /*!
     * according to the compress procedure, each byte(two hexadecimal digits) should be reversed for each hexadecimal
     */
    unsigned char *bit_stream = (unsigned char *) calloc(content.len * 4, sizeof(unsigned char));
    for (int i = 0; i < content.len / 2; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            bit_stream[content.len * 4 - 5 - 8 * i - j] =
                    ((content.value[2 * i] < 58) ? content.value[2 * i] - 48 : content.value[2 * i] - 55) >> j & 0x01;
            bit_stream[content.len * 4 - 1 - 8 * i - j] =
                    ((content.value[2 * i + 1] < 58) ? content.value[2 * i + 1] - 48 : content.value[2 * i + 1] - 55)
                            >> j & 0x01;
        }
    }

    return bit_stream;
}

/*!
 * @brief: handle an ordinal
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 */
void handle_ordinal(DSELF *self, STRUCTRESULT *result_decompress)
{
    // get the current ordinal and add it to decode result
    unsigned char cur_ordinal = get_code(self, self->params[1][4], self->index_message_compressed);
    update_result_decompress(result_decompress, cur_ordinal);

    unsigned char length_pre_string = 0;
    unsigned int pre_codeword = 0;
    STRINGCOLLECTION pre_string_collection = {0, 0, 0};
    if ((unsigned int) self->flag_pre_code == 0) // previous code is an ordinal
    {
        // the string represented by the new string_collection is the previous ordinal and the current ordinal
        new_string_collection(self, result_decompress->len - 1, 2);
    }
    else if ((unsigned int) self->flag_pre_code == 1) // previous code is a codeword
    {
        // get the previous codeword and the string_collection with it
        pre_codeword = get_code(self, self->params[1][1],
                                (unsigned int) self->index_message_compressed + 2 + self->params[1][1]);
        pre_string_collection = search_string_collection_by_codeword(self, pre_codeword);
        length_pre_string = pre_string_collection.string_length;

        // the string represented by the new string_collection is the string of the previous codeword and the current ordinal
        new_string_collection(self, result_decompress->len - 1, length_pre_string + 1);
    }
    else if ((unsigned int) self->flag_pre_code == 4)
    {
        if ((unsigned int) self->flag_first_two_code == 0)
        {
            new_string_collection(self, result_decompress->len - 1, 2);
        }
        else if ((unsigned int) self->flag_first_two_code == 1)
        {
            pre_codeword = get_code(self, self->params[1][1],
                                    (unsigned int) self->index_message_compressed + 2 + 2 * self->params[1][1]);
            pre_string_collection = search_string_collection_by_codeword(self, pre_codeword);
            length_pre_string = pre_string_collection.string_length;

            new_string_collection(self, result_decompress->len - 1, length_pre_string + 1);
        }
    }

    self->index_message_compressed -= self->params[1][4] + 1;
}

/*!
 * @brief:handle a codeword
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 * @param cur_code: the code to be handled
 */
void handle_codeword(DSELF *self, STRUCTRESULT *result_decompress, unsigned int cur_code)
{
    STRINGCOLLECTION cur_string_collection = {0, 0, 0}, pre_string_collection = {0, 0, 0};
    unsigned int first_char_pos_pre_string = 0, codeword_pre_string = 0, pre_ordinal = 0;

    // if current codeword is less than the C1, which means the string_collection with the codeword has been created
    if ((unsigned int) cur_code < self->params[1][0])
    {
        // get the string_collection with current codeword and update decompress result with the string represented by current codeword
        cur_string_collection = search_string_collection_by_codeword(self, cur_code);
        first_char_pos_pre_string = 1 + cur_string_collection.last_char_pos - cur_string_collection.string_length;
        for (int i = 0; i < (unsigned int) cur_string_collection.string_length; ++i)
        {
            update_result_decompress(result_decompress, result_decompress->result[first_char_pos_pre_string + i]);
        }

        // according to the flag that indicates the type of previous code to create the string_collection
        if ((unsigned int) self->flag_pre_code == 0)
        {
            // previous code is an ordinal, the string that the string_collection should be the ordinal and the first character of the current codeword
            new_string_collection(self, result_decompress->len - cur_string_collection.string_length, 2);
        }
        else if ((unsigned int) self->flag_pre_code == 1)
        {
            // the previous code is a codeword
            codeword_pre_string = get_code(self, self->params[1][1],
                                           (unsigned int) self->index_message_compressed + 2 +
                                           self->params[1][1] * 2);
            pre_string_collection = search_string_collection_by_codeword(self, codeword_pre_string);

            // the string should be the string of previous codeword and the first character of the current codeword
            new_string_collection(self, result_decompress->len - cur_string_collection.string_length,
                                  (unsigned int) pre_string_collection.string_length + 1);
        }
        else if ((unsigned int) self->flag_pre_code == 4) // if the previous code is the SETUP code
        {
            if ((unsigned int) self->flag_first_two_code == 0)
            {
                new_string_collection(self, result_decompress->len - cur_string_collection.string_length, 2);
            }
            else if ((unsigned int) self->flag_first_two_code == 1)
            {
                // In this case, the current codeword immediately follows the codeword SETUP control code, and the codeword SETUP control code follows the other codeword.
                codeword_pre_string = get_code(self, self->params[1][1] - 1,
                                               (unsigned int) self->index_message_compressed + 1 +
                                               3 * self->params[1][1]);
                pre_string_collection = search_string_collection_by_codeword(self, codeword_pre_string);

                new_string_collection(self, result_decompress->len - cur_string_collection.string_length,
                                      (unsigned int) pre_string_collection.string_length + 1);
            }
        }
    }
        // current code is equal to the C1, which means the string_collection with the codeword is not created yet
    else if ((unsigned int) cur_code == self->params[1][0])
    {
        // the previous code is an ordinal
        if ((unsigned int) self->flag_pre_code == 0)
        {
            // get the previous ordinal and add it to decode result twice
            pre_ordinal = get_code(self, self->params[1][4],
                                   (unsigned int) self->index_message_compressed + 2 + self->params[1][1] +
                                   self->params[1][4]);
            for (int i = 0; i < 2; ++i)
            {
                update_result_decompress(result_decompress, pre_ordinal);
            }

            // the string represented by the string_collection is a double character of the ordinal
            new_string_collection(self, result_decompress->len - 2, 2);
        }
        else if ((unsigned int) self->flag_pre_code == 1)
        {
            // get previous codeword and the string_collection with it
            codeword_pre_string = get_code(self, self->params[1][1],
                                           (unsigned int) self->index_message_compressed + 2 +
                                           2 * self->params[1][1]);
            pre_string_collection = search_string_collection_by_codeword(self, codeword_pre_string);
            first_char_pos_pre_string = pre_string_collection.last_char_pos - pre_string_collection.string_length + 1;

            // add the string represented by the previous codeword and the first character of the string of the previous codeword to decode result
            for (int i = 0; i < (unsigned int) pre_string_collection.string_length; ++i)
            {
                update_result_decompress(result_decompress, result_decompress->result[first_char_pos_pre_string + i]);
            }
            update_result_decompress(result_decompress, result_decompress->result[first_char_pos_pre_string]);

            // the string represented by the new string_collection is the string represented by the previous codeword and the first character of it
            new_string_collection(self, result_decompress->len - cur_string_collection.string_length,
                                  (unsigned int) pre_string_collection.string_length + 1);
        }
    }
}

/*!
 * @brief: handle a string-extension length
 * @param self: the struct to save data
 * @param result_decompress: the struct to save the encode result
 */
void handle_string_extension_length(DSELF *self, STRUCTRESULT *result_decompress)
{
    // get the bit_cost that is cost on representing the string-extension length and the string-extension length
    unsigned int result_transfer = transfer_string_extension_length(self);
    unsigned int bit_cost = result_transfer >> 16 & 0xffff;
    unsigned int length_seg = result_transfer & 0xffff;

    // get the codeword transferred before the string-extension length and get the string_collection saved it
    unsigned int pre_codeword = get_code(self, self->params[1][1],
                                         2 + bit_cost + self->params[1][1] + self->index_message_compressed);
    STRINGCOLLECTION pre_string_collection = search_string_collection_by_codeword(self, pre_codeword);
    unsigned int last_char_pos_pre_string = pre_string_collection.last_char_pos;

    // update decompress result with the character according to the last character position
    while (length_seg > 0)
    {
        update_result_decompress(result_decompress, result_decompress->result[last_char_pos_pre_string + 1]);
        last_char_pos_pre_string++;
        length_seg--;
    }

    // create the new string_collection to record the string extended, the string represented by the new string_collection is the string represented by the previous codeword and the string represented by the string-extension length
    new_string_collection(self, result_decompress->len - 1,
                          pre_string_collection.string_length + result_transfer & 0xffff);

    self->index_message_compressed--;
}

/*!
 * @brief: handle a control code
 * @param self: the struct to save data
 * @param cur_code: the code to be handled
 */
void handle_control_code(DSELF *self, unsigned int cur_code)
{
    if (cur_code == 1) // FLUSH
    {
        // calculate the number of bits that are just added while compress
        unsigned int bit_decoded = (unsigned int) self->len_message_compressed - self->index_message_compressed - 1;
        self->index_message_compressed = bit_decoded % 8 == 0 ? self->index_message_compressed : bit_decoded % 8 - 8 +
                                                                                                 self->index_message_compressed;
    }
    else if (cur_code == 2) // SETUP
    {
        // if the bit after current code is zero(0), the SETUP control code is transferred for the param that controls the length of ordinal, otherwise it is for param for codeword
        if (self->message_compressed[self->index_message_compressed] == 0) // next code is an ordinal
        {
            self->params[1][4]++;
        }
        else // next code is a codeword
        {
            self->params[1][1]++;
            self->params[1][2] *= 2;
        }
    }
//    else if (cur_code == 0) // ETM
//    {
//
//    }
//    else if (cur_code == 3) // REINIT
//    {
//
//    }
}

/*!
 * @brief: create a new struct of string_collection
 * @param self: the struct to save data
 * @param last_char_pos: the last character position in the decode history
 * @param string_length: the length of the string represented by the string_collection
 */
void new_string_collection(DSELF *self, unsigned int last_char_pos, unsigned int string_length)
{
    STRINGCOLLECTION tmp_string_collection = {last_char_pos, self->params[1][0], string_length};
    self->array_string_collection[self->count_string_collection] = tmp_string_collection;

    self->params[1][0]++;
    self->count_string_collection++;
}

/*!
 * @brief: traverse the array of the string_collection by the codeword searched to get the string_collection whose codeword is equal to the codeword searched
 * @param self: the struct to save data
 * @param codeword_searched: the codeword to be searched
 * @return: the string_collection whose codeword is equal to the codeword searched
 */
STRINGCOLLECTION search_string_collection_by_codeword(DSELF *self, unsigned int codeword_searched)
{
    STRINGCOLLECTION string_collection = {0, 0, 0};
    for (int i = 0; i < (unsigned int) self->count_string_collection; ++i)
    {
        if (self->array_string_collection[i].codeword == codeword_searched)
        {
            string_collection = self->array_string_collection[i];
        }
    }

    return string_collection;
}

/*!
 * @brief: transfer 1 to up to 12 bits to get the string-extension length in integer
 * @param self: the struct to save data
 * @return: the string-extension length
 */
unsigned int transfer_string_extension_length(DSELF *self)
{
    /*
     * the value has two part.
     * In bit, the lower part means the bit_cost how many bits is cost to represent the string-extension-length
     *         the higher part means the string-extension-length in integer
     */
    unsigned int value = 0;
    self->index_message_compressed--;
    if (self->message_compressed[self->index_message_compressed] == 1)
    {
        // the string-extension-length = 1
        value = 0x00010000;
        value += 1;
    }
    else
    {
        // move the index forward two bits
        self->index_message_compressed -= 2;

        // if the two bits are not zero(0) at the same time
        if (self->message_compressed[(unsigned int) self->index_message_compressed] |
            self->message_compressed[(unsigned int) self->index_message_compressed + 1])
        {
            // the string-extension-length in [2, 4]
            value = 0x00030000;
            value += get_code(self, 2, 2 + (unsigned int) self->index_message_compressed) + 1;
        }
        else
        {
            // move the index forward four bits
            self->index_message_compressed -= 4;

            // if the bit represented the different branches is zero
            if (self->message_compressed[(unsigned int) self->index_message_compressed + 3] == 0)
            {
                // the string-extension-length in [5, 12]
                value = 0x00070000;
                value += get_code(self, 3, 3 + (unsigned int) self->index_message_compressed) + 5;
            }
            else
            {
                // according to the param N7 to handle the string-extension length
                // the string-extension-length in [13, 255]
                if (32 <= self->params[0][6] && self->params[0][6] <= 46)
                {
                    self->index_message_compressed -= 2;
                    value = 0x00090000;
                    value += get_code(self, 5, 5 + (unsigned int) self->index_message_compressed) + 13;
                }
                else if (46 < self->params[0][6] && self->params[0][6] <= 78)
                {
                    self->index_message_compressed -= 3;
                    value = 0x000A0000;
                    value += get_code(self, 6, 6 + (unsigned int) self->index_message_compressed) + 13;
                }
                else if (78 < self->params[0][6] && self->params[0][6] <= 142)
                {
                    self->index_message_compressed -= 4;
                    value = 0x000B0000;
                    value += get_code(self, 7, 7 + (unsigned int) self->index_message_compressed) + 13;
                }
                else if (142 < self->params[0][6] && self->params[0][6] <= 255)
                {
                    self->index_message_compressed -= 5;
                    value = 0x000C0000;
                    value += get_code(self, 8, 8 + (unsigned int) self->index_message_compressed) + 13;
                }
            }
        }
    }

    return value;
}

/*!
 * @brief: save the decode result in the struct of result
 * @param result_decompress: the struct to save result
 * @param result_added: the result to be added
 */
void update_result_decompress(STRUCTRESULT *result_decompress, unsigned char result_added)
{
//    printf("%02X ", code_added);
//    for (int i = 0; i < 8; ++i)
//    {
//        printf("%d", code_added >> (7 - i) & 0x01);
//    }
//    printf("\n");
    result_decompress->result[result_decompress->len] = result_added;
    result_decompress->len++;
}

/*!
 * @brief: write the decompress result with the decompress result
 * @param result_decompress: the struct to save result
 */
void write_decompress_result_file(STRUCTRESULT result_decompress)
{
    FILE *fp = fopen("messageDecompressResult.txt", "w");
    if (fp == NULL)
    {
        perror("Fopen(messageDecompressResult.txt) error:");

        return;
    }


    // set an unsigned char to two digit hexadecimal number
    for (int i = 0; i < result_decompress.len; ++i)
    {
        fprintf(fp, "%02X", result_decompress.result[i]);
    }

    fclose(fp);
    fp = NULL;
}