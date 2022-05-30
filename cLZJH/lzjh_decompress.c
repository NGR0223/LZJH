//
// Created by ngr02 on 2021/7/7.
//

#include "lzjh_decompress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned int SelfParam[2][8] = {{10, 1024, 8,  256, 4, 0, 255, 3072},
                                      {4,  6,    64, 0,   7}};
DSELF Self = {{{10, 1024, 8, 256, 4, 0, 255, 3072}, {4, 6, 64, 0, 7}}, NULL, {0}, 0, 0, 0, 4, 7};

STRUCTRESULT ResultDecompress = {0, {0}};

/*!
 * @brief: the decompress function includes the decode function and the writing result to file function
 */
void decompress()
{
    // decode
    decode();

    // write the file with the result
    write_decompress_result_file();
    puts("Decompressed successfully");

    // Reset Self and ResultDecompress
    memcpy(Self.params, SelfParam, sizeof(SelfParam));
    free(Self.message_compressed);
    Self.message_compressed = NULL;
    memset(Self.array_string_collection, 0, sizeof(STRINGCOLLECTION) * 1024);
    Self.index_message_compressed = 0;
    Self.len_message_compressed = 0;
    Self.count_string_collection = 0;
    Self.flag_pre_code = 4;
    Self.flag_first_two_code = 7;
    ResultDecompress.len = 0;
    memset(ResultDecompress.result, 0, RESULT_SIZE);
}

/*!
 * @brief: transfer a bit string to an unsigned integer decimal number
 * @param length: the length of the bit string
 * @param last_bit_pos: the last position of the bit string in the message_compressed
 * @return: the unsigned integer decimal number of the bit string
 */
static unsigned int get_code(unsigned char length, unsigned int last_bit_pos)
{
    unsigned int cur_code = 0;

    for (int i = 0; i < length; ++i)
    {
        cur_code += Self.message_compressed[(unsigned int) last_bit_pos - 1 - i] << i;
    }

    return cur_code;
}

/*!
 * @brief: the decode function to recover the message_compressed
 */
void decode()
{
    // get message compressed
    STRUCTRETURN struct_return = read_file_bin(MESSAGE_COMPRESSED_FILE_PATH);

    //Init data structure and other variables
    Self.index_message_compressed = struct_return.len * 4 - 1;
    Self.len_message_compressed = struct_return.len * 4;
    Self.message_compressed = set_content_to_bit_stream(struct_return);
    free(struct_return.value);
    struct_return.value = NULL;

    unsigned int cur_code = 0;
    while (Self.index_message_compressed < Self.len_message_compressed)
    {
        // the first bit must be a prefix, according to the first bit to determine the type of the following code
        if (Self.index_message_compressed == Self.len_message_compressed - 1)
        {
            // the first bit is zero(0), so the code is an ordinal
            if (Self.message_compressed[Self.index_message_compressed] == 0)
            {
                handle_ordinal();
                Self.flag_pre_code = 0;
            }
            else
            {
                // the first bit is one(1), so the code is a code(codeword or control code)
                cur_code = get_code(Self.params[1][1], Self.index_message_compressed);
                Self.index_message_compressed -= Self.params[1][1] + 1;
                //  if the value of the code is less than 4(0~3), the code is a control code; otherwise, the code is a codeword
                if (cur_code < 4)
                {
                    handle_control_code(cur_code);
                    Self.flag_first_two_code =
                            Self.flag_pre_code > 2 ? Self.flag_first_two_code : Self.flag_pre_code;
                    Self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                }
                else
                {
                    handle_codeword(cur_code);
                    Self.flag_pre_code = 1;
                }
            }
            continue;
        }
        // if the first code has been handled, the remaining codes depend on the flag of previous code to decide how to handle them
        // see the Chapter 6.6.3(P15) of the file(V.44 Data Compression Procedures) for specific rules
        // the previous code is a codeword
        if ((unsigned int) Self.flag_pre_code == 1)
        {
            // the current code is a codeword or a control code
            if (Self.message_compressed[Self.index_message_compressed] == 1)
            {
                cur_code = get_code(Self.params[1][1], Self.index_message_compressed);
                Self.index_message_compressed -= Self.params[1][1] + 1;
                if (cur_code < 4)
                {
                    handle_control_code(cur_code);
                    Self.flag_first_two_code =
                            Self.flag_pre_code > 2 ? Self.flag_first_two_code : Self.flag_pre_code;
                    Self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                }
                else
                {
                    handle_codeword(cur_code);
                    Self.flag_pre_code = 1;
                }
            }
            else
            {
                Self.index_message_compressed--;
                // the current code is an ordinal or a string-extension length
                if (Self.message_compressed[Self.index_message_compressed] == 1)
                {
                    handle_string_extension_length();
                    Self.flag_pre_code = 2;
                }
                else
                {
                    handle_ordinal();
                    Self.flag_pre_code = 0;
                }
            }
        }
        else // the previous code is other codes
        {
            // the current code is a codeword or a control code
            if (Self.message_compressed[Self.index_message_compressed] == 1)
            {
                cur_code = get_code(Self.params[1][1], Self.index_message_compressed);
                Self.index_message_compressed -= Self.params[1][1] + 1;
                if (cur_code < 4)
                {
                    handle_control_code(cur_code);
                    Self.flag_first_two_code =
                            Self.flag_pre_code > 2 ? Self.flag_first_two_code : Self.flag_pre_code;
                    Self.flag_pre_code = cur_code == 1 ? 3 : 4; // to distinguish the two control codes
                }
                else
                {
                    handle_codeword(cur_code);
                    Self.flag_pre_code = 1;
                }
            }
            else // the current code is an ordinal
            {
                handle_ordinal();
                Self.flag_pre_code = 0;
            }
        }
    }
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
 * @brief create new string collection when the previous code is a codeword
 * @param length_cur_string length of current string, if cur code is ordinal, it is 1
 * @param type_cur_code type of current code
 */
static void handle_previous_codeword(unsigned int length_cur_string, unsigned int type_cur_code)
{
    unsigned int length_pre_code = Self.params[1][1];
    unsigned int deviation = 0;
    if (type_cur_code == 0)
    {
        if (Self.flag_pre_code == 1)
        {
            deviation = 2 + Self.params[1][1];
        }
        else
        {
            deviation = 2 + 2 * Self.params[1][1];
        }
    }
    else
    {
        if (Self.flag_pre_code == 1)
        {
            deviation = 2 + 2 * Self.params[1][1];
        }
        else
        {
            length_pre_code--;
            deviation = 1 + 3 * Self.params[1][1];
        }
    }

    // get the previous codeword and the string_collection with it
    unsigned int pre_codeword = get_code(length_pre_code, Self.index_message_compressed + deviation);
    STRINGCOLLECTION pre_string_collection = search_string_collection_by_codeword(pre_codeword);

    // the string represented by the new string_collection is the string of the previous codeword and the current ordinal
    new_string_collection(ResultDecompress.len - length_cur_string, pre_string_collection.string_length + 1);
}

/*!
 * @brief: handle an ordinal
 */
void handle_ordinal()
{
    // get the current ordinal and add it to decode result
    unsigned char cur_ordinal = get_code(Self.params[1][4], Self.index_message_compressed);
    update_result_decompress(cur_ordinal);

    if (Self.flag_pre_code == 0) // previous code is an ordinal
    {
        // the string represented by the new string_collection is the previous ordinal and the current ordinal
        new_string_collection(ResultDecompress.len - 1, 2);
    }
    else if (Self.flag_pre_code == 1) // previous code is a codeword
    {
        handle_previous_codeword(1, 0);
    }
    else if (Self.flag_pre_code == 4)
    {
        if (Self.flag_first_two_code == 0)
        {
            new_string_collection(ResultDecompress.len - 1, 2);
        }
        else if (Self.flag_first_two_code == 1)
        {
            handle_previous_codeword(1, 0);
        }
    }

    Self.index_message_compressed -= Self.params[1][4] + 1;
}

/*!
 * @brief:handle a codeword
 * @param cur_code: the code to be handled
 */
void handle_codeword(unsigned int cur_code)
{
    STRINGCOLLECTION cur_string_collection = {0, 0, 0}, pre_string_collection = {0, 0, 0};
    unsigned int first_char_pos_pre_string = 0, codeword_pre_string = 0, pre_ordinal = 0;

    // if current codeword is less than the C1, which means the string_collection with the codeword has been created
    if ((unsigned int) cur_code < Self.params[1][0])
    {
        // get the string_collection with current codeword and update decompress result with the string represented by current codeword
        cur_string_collection = search_string_collection_by_codeword(cur_code);
        first_char_pos_pre_string = 1 + cur_string_collection.last_char_pos - cur_string_collection.string_length;
        for (int i = 0; i < (unsigned int) cur_string_collection.string_length; ++i)
        {
            update_result_decompress(ResultDecompress.result[first_char_pos_pre_string + i]);
        }

        // according to the flag that indicates the type of previous code to create the string_collection
        if ((unsigned int) Self.flag_pre_code == 0)
        {
            // previous code is an ordinal, the string that the string_collection should be the ordinal and the first character of the current codeword
            new_string_collection(ResultDecompress.len - cur_string_collection.string_length, 2);
        }
        else if ((unsigned int) Self.flag_pre_code == 1)
        {
            // the previous code is a codeword
            handle_previous_codeword(cur_string_collection.string_length, 1);
        }
        else if ((unsigned int) Self.flag_pre_code == 4) // if the previous code is the SETUP code
        {
            if ((unsigned int) Self.flag_first_two_code == 0)
            {
                new_string_collection(ResultDecompress.len - cur_string_collection.string_length, 2);
            }
            else if ((unsigned int) Self.flag_first_two_code == 1)
            {
                // In this case, the current codeword immediately follows the codeword SETUP control code, and the codeword SETUP control code follows the other codeword.
                handle_previous_codeword(cur_string_collection.string_length, 1);
            }
        }
    }
        // current code is equal to the C1, which means the string_collection with the codeword is not created yet
    else if ((unsigned int) cur_code == Self.params[1][0])
    {
        // the previous code is an ordinal
        if ((unsigned int) Self.flag_pre_code == 0)
        {
            // get the previous ordinal and add it to decode result twice
            pre_ordinal = get_code(Self.params[1][4],
                                   Self.index_message_compressed + 2 + Self.params[1][1] + Self.params[1][4]);
            for (int i = 0; i < 2; ++i)
            {
                update_result_decompress(pre_ordinal);
            }

            // the string represented by the string_collection is a double character of the ordinal
            new_string_collection(ResultDecompress.len - 2, 2);
        }
        else if ((unsigned int) Self.flag_pre_code == 1)
        {
            // get previous codeword and the string_collection with it
            codeword_pre_string = get_code(Self.params[1][1],
                                           Self.index_message_compressed + 2 + 2 * Self.params[1][1]);
            pre_string_collection = search_string_collection_by_codeword(codeword_pre_string);
            first_char_pos_pre_string = pre_string_collection.last_char_pos - pre_string_collection.string_length + 1;

            // add the string represented by the previous codeword and the first character of the string of the previous codeword to decode result
            for (int i = 0; i < pre_string_collection.string_length; ++i)
            {
                update_result_decompress(ResultDecompress.result[first_char_pos_pre_string + i]);
            }
            update_result_decompress(ResultDecompress.result[first_char_pos_pre_string]);

            // the string represented by the new string_collection is the string represented by the previous codeword and the first character of it
            new_string_collection(ResultDecompress.len - 2,
                                  pre_string_collection.string_length + 1);
        }
        else if ((unsigned int) Self.flag_pre_code == 4) // if the previous code is the SETUP code
        {
            if ((unsigned int) Self.flag_first_two_code == 0)
            {
                // get the previous ordinal and add it to decode result twice
                pre_ordinal = get_code(Self.params[1][4],
                                       Self.index_message_compressed + 2 + 2 * Self.params[1][1] + Self.params[1][4]);
                for (int i = 0; i < 2; ++i)
                {
                    update_result_decompress(pre_ordinal);
                }
                new_string_collection(ResultDecompress.len - 2, 2);
            }
            else if ((unsigned int) Self.flag_first_two_code == 1)
            {
                // get previous codeword and the string_collection with it
                codeword_pre_string = get_code(Self.params[1][1] - 1,
                                               Self.index_message_compressed + 1 + 3 * Self.params[1][1]);
                pre_string_collection = search_string_collection_by_codeword(codeword_pre_string);
                first_char_pos_pre_string =
                        pre_string_collection.last_char_pos - pre_string_collection.string_length + 1;

                // add the string represented by the previous codeword and the first character of the string of the previous codeword to decode result
                for (int i = 0; i < (unsigned int) pre_string_collection.string_length; ++i)
                {
                    update_result_decompress(ResultDecompress.result[first_char_pos_pre_string + i]);
                }
                update_result_decompress(ResultDecompress.result[first_char_pos_pre_string]);

                // In this case, the current codeword immediately follows the codeword SETUP control code, and the codeword SETUP control code follows the codeword.
                new_string_collection(ResultDecompress.len - 2, (unsigned int) pre_string_collection.string_length + 1);
            }
        }
    }
}

/*!
 * @brief: handle a string-extension length
 */
void handle_string_extension_length()
{
    // get the bit_cost that is cost on representing the string-extension length and the string-extension length
    unsigned int result_transfer = transfer_string_extension_length();
    unsigned int bit_cost = result_transfer >> 16 & 0xffff;
    unsigned int length_seg = result_transfer & 0xffff;

    // get the codeword transferred before the string-extension length and get the string_collection saved it
    unsigned int pre_codeword = get_code(Self.params[1][1],
                                         2 + bit_cost + Self.params[1][1] + Self.index_message_compressed);
    STRINGCOLLECTION pre_string_collection = search_string_collection_by_codeword(pre_codeword);
    unsigned int last_char_pos_pre_string = pre_string_collection.last_char_pos;

    // update decompress result with the character according to the last character position
    while (length_seg > 0)
    {
        update_result_decompress(ResultDecompress.result[last_char_pos_pre_string + 1]);
        last_char_pos_pre_string++;
        length_seg--;
    }

    // create the new string_collection to record the string extended, the string represented by the new string_collection is the string represented by the previous codeword and the string represented by the string-extension length
    new_string_collection(ResultDecompress.len - 1,
                          pre_string_collection.string_length + result_transfer & 0xffff);

    Self.index_message_compressed--;
}

/*!
 * @brief: handle a control code
 * @param cur_code: the code to be handled
 */
void handle_control_code(unsigned int cur_code)
{
    if (cur_code == 1) // FLUSH
    {
        // calculate the number of bits that are just added while compress
        unsigned int bit_decoded = (unsigned int) Self.len_message_compressed - Self.index_message_compressed - 1;
        Self.index_message_compressed = bit_decoded % 8 == 0 ? Self.index_message_compressed : bit_decoded % 8 - 8 +
                                                                                               Self.index_message_compressed;
    }
    else if (cur_code == 2) // SETUP
    {
        // if the bit after current code is zero(0), the SETUP control code is transferred for the param that controls the length of ordinal, otherwise it is for param for codeword
        if (Self.message_compressed[Self.index_message_compressed] == 0) // next code is an ordinal
        {
            Self.params[1][4]++;
        }
        else // next code is a codeword
        {
            Self.params[1][1]++;
            Self.params[1][2] *= 2;
        }
    }
}

/*!
 * @brief: create a new struct of string_collection
 * @param last_char_pos: the last character position in the decode history
 * @param string_length: the length of the string represented by the string_collection
 */
void new_string_collection(unsigned int last_char_pos, unsigned int string_length)
{
    STRINGCOLLECTION tmp_string_collection = {last_char_pos, Self.params[1][0], string_length};
    Self.array_string_collection[Self.count_string_collection] = tmp_string_collection;

    Self.params[1][0]++;
    Self.count_string_collection++;
}

/*!
 * @brief: traverse the array of the string_collection by the codeword searched to get the string_collection whose codeword is equal to the codeword searched
 * @param codeword_searched: the codeword to be searched
 * @return: the string_collection whose codeword is equal to the codeword searched
 */
STRINGCOLLECTION search_string_collection_by_codeword(unsigned int codeword_searched)
{
    STRINGCOLLECTION string_collection = {0, 0, 0};
    for (int i = 0; i < (unsigned int) Self.count_string_collection; ++i)
    {
        if (Self.array_string_collection[i].codeword == codeword_searched)
        {
            string_collection = Self.array_string_collection[i];
        }
    }

    return string_collection;
}

/*!
 * @brief: transfer 1 to up to 12 bits to get the string-extension length in integer
 * @return: the string-extension length
 */
unsigned int transfer_string_extension_length()
{
    /*
     * the value has two part.
     * In bit, the lower part means the bit_cost how many bits is cost to represent the string-extension-length
     *         the higher part means the string-extension-length in integer
     */
    unsigned int value = 0;
    Self.index_message_compressed--;
    if (Self.message_compressed[Self.index_message_compressed] == 1)
    {
        // the string-extension-length = 1
        value = 0x00010000;
        value += 1;
    }
    else
    {
        // move the index forward two bits
        Self.index_message_compressed -= 2;

        // if the two bits are not zero(0) at the same time
        if (Self.message_compressed[(unsigned int) Self.index_message_compressed] |
            Self.message_compressed[(unsigned int) Self.index_message_compressed + 1])
        {
            // the string-extension-length in [2, 4]
            value = 0x00030000;
            value += get_code(2, 2 + Self.index_message_compressed) + 1;
        }
        else
        {
            // move the index forward four bits
            Self.index_message_compressed -= 4;

            // if the bit represented the different branches is zero
            if (Self.message_compressed[(unsigned int) Self.index_message_compressed + 3] == 0)
            {
                // the string-extension-length in [5, 12]
                value = 0x00070000;
                value += get_code(3, 3 + Self.index_message_compressed) + 5;
            }
            else
            {
                // according to the param N7 to handle the string-extension length
                // the string-extension-length in [13, 255]
                if (32 <= Self.params[0][6] && Self.params[0][6] <= 46)
                {
                    Self.index_message_compressed -= 2;
                    value = 0x00090000;
                    value += get_code(5, 5 + Self.index_message_compressed) + 13;
                }
                else if (46 < Self.params[0][6] && Self.params[0][6] <= 78)
                {
                    Self.index_message_compressed -= 3;
                    value = 0x000A0000;
                    value += get_code(6, 6 + Self.index_message_compressed) + 13;
                }
                else if (78 < Self.params[0][6] && Self.params[0][6] <= 142)
                {
                    Self.index_message_compressed -= 4;
                    value = 0x000B0000;
                    value += get_code(7, 7 + (Self.index_message_compressed) + 13);
                }
                else if (142 < Self.params[0][6] && Self.params[0][6] <= 255)
                {
                    Self.index_message_compressed -= 5;
                    value = 0x000C0000;
                    value += get_code(8, 8 + Self.index_message_compressed) + 13;
                }
            }
        }
    }

    return value;
}

/*!
 * @brief: save the decode result in the struct of result
 * @param result_added: the result to be added
 */
void update_result_decompress(unsigned char result_added)
{
    ResultDecompress.result[ResultDecompress.len] = result_added;
    ResultDecompress.len++;
}

/*!
 * @brief: write the decompress result with the decompress result
 */
void write_decompress_result_file()
{
    FILE *fp = fopen("messageDecompressResult.txt", "w");
    if (fp == NULL)
    {
        perror("Fopen(messageDecompressResult.txt) error:");

        return;
    }

    // set an unsigned char to two digit hexadecimal number
    for (int i = 0; i < ResultDecompress.len; ++i)
    {
        fprintf(fp, "%02X", ResultDecompress.result[i]);
    }

    fclose(fp);
    fp = NULL;
}