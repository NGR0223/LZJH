//
// Created by ngr02 on 2021/6/20.
//

#include "lzjh_compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
 * @brief:  the compress function includes the encode function and the writing result to file function
 */
void compress()
{
    STRUCTRESULT resultCompress = {0, {0}};

    encode(&resultCompress);

    // write the file with the result
    write_compress_result_file(resultCompress);
    puts("Compressed successfully");
}

/*!
 * @brief: encode the file with message
 * @param result_compress: the struct to save the result of compress
 */
void encode(STRUCTRESULT *result_compress)
{
    // get message
    STRUCTRETURN struct_content = read_file_bin(MESSAGE_FILE_PATH);

    // init data structure and other variables
    CSELF self = {{256, {0}}, {{10, 1024, 8, 256, 4, 0, 255, 3072},
                               {4, 6, 64, 0, 7}}, NULL, struct_content.len / 2, 0, 15};
    // init the root_array
    for (int i = 0; i < 256; ++i)
    {
        init_node(&self.array_node.arr_node[i], 0, 0, 1);
    }
    self.message = set_content_to_message(struct_content);

    free(struct_content.value);
    struct_content.value = NULL;

    // traverse the message
    while ((unsigned int) self.index_message < self.len_message)
    {
        unsigned char cur_char = self.message[self.index_message];
        self.index_message++;

        // whether the current char appears for the first time
        if (self.array_node.arr_node[cur_char].down_index == (unsigned int) 0)
        {
            if (cur_char >> self.params[1][4]) // whether the bit of current char is more than C5
            {
                self.params[1][4]++;

                // handle prefix of ordinal SETUP code
                update_result_compress(result_compress, &self, 3, 0);

                //handle the ordinal SETUP code
                update_result_compress(result_compress, &self, 2, 2);

                self.flag_pre_code = 3;
            }
            // handle a new character
            new_character(cur_char, &self);

            // handle the prefix of the ordinal
            update_result_compress(result_compress, &self, 0, 0);

            // handle the ordinal
            update_result_compress(result_compress, &self, cur_char, 1);

            self.flag_pre_code = 0;
        }
        else
        {
            // try to match the possible longest string
            MATCHRESULT match_result = {0, NULL};
            match_longest_string(self.message, (unsigned int) self.index_message, &match_result, &self.array_node,
                                 self.len_message);
            self.index_message += match_result.len_matched;

            // whether the encoder should process the string-extension procedure
            if (match_result.len_matched == 0) // the string-matching procedure only matched the root node
            {
                if (cur_char >> self.params[1][4]) // whether the bit of current char is more than C5
                {
                    self.params[1][4]++;

                    // handle the prefix of the ordinal SETUP code
                    update_result_compress(result_compress, &self, 3, 0);

                    //handle the ordinal SETUP code
                    update_result_compress(result_compress, &self, 2, 2);

                    self.flag_pre_code = 3;
                }
                // handle the prefix of the ordinal
                update_result_compress(result_compress, &self, 0, 0);

                // handle the ordinal
                update_result_compress(result_compress, &self, self.message[(unsigned int) self.index_message - 1], 1);

                self.flag_pre_code = 0;

                // add the next character to the list of children of the root_array[cur_char]
                if ((unsigned int) self.index_message < self.len_message)
                {
                    NODE *tmp_node = (NODE *) calloc(1, sizeof(NODE));

                    init_node(tmp_node, self.params[1][0], self.index_message, 1);
                    add_child(&self.array_node.arr_node[self.message[(unsigned int) self.index_message - 1]],
                              tmp_node, &self.array_node);
                    self.params[1][0]++;

                    free(tmp_node);
                    tmp_node = NULL;
                }
            }
            else
            {
                // whether the bit of current codeword is more than C2
                while ((unsigned int) match_result.node_matched->codeword >> self.params[1][1])
                {
                    // handle the prefix of the codeword SETUP code
                    update_result_compress(result_compress, &self, 3, 0);

                    //handle the codeword SETUP code
                    update_result_compress(result_compress, &self, 2, 2);

                    self.params[1][1]++;
                    self.params[1][2] *= 2;

                    self.flag_pre_code = 3;
                }
                // handle the prefix of the codeword
                update_result_compress(result_compress, &self, 1, 0);

                // handle the codeword
                update_result_compress(result_compress, &self, match_result.node_matched->codeword, 2);

                self.flag_pre_code = 1;
                if ((unsigned int) self.index_message < self.len_message)
                {
                    unsigned int len_string_extension = extend_string_segment(&self, match_result.node_matched);
                    if (len_string_extension != 0)
                    {
                        // handle the prefix of the string-extension length
                        update_result_compress(result_compress, &self, 2, 0);

                        // handle the string-extension length
                        update_result_compress(result_compress, &self, len_string_extension, 3);

                        self.flag_pre_code = 2;
                    }
                }
            }
        }
    }
    // handle the prefix of the FLUSH code
    update_result_compress(result_compress, &self, 3, 0);

    // handle the FLUSH code
    update_result_compress(result_compress, &self, 1, 2);

    self.flag_pre_code = 3;

    free(self.message);
    self.message = NULL;
}

/*!
 * @brief: because the content of the file is hexadecimal numbers, the encode procedure should transform two hexadecimal digits to an unsigned char
 * @param content: the content of the file
 * @return: the point of the unsigned char string that has been transformed
 */
unsigned char *set_content_to_message(STRUCTRETURN content)
{
    unsigned char *message = (unsigned char *) calloc(content.len / 2, sizeof(unsigned));

    for (int i = 0; i < content.len / 2; ++i)
    {
        message[i] = ((content.value[2 * i] < 58) ? content.value[2 * i] - 48 : content.value[2 * i] - 55) * 16 +
                     ((content.value[2 * i + 1] < 58) ? content.value[2 * i + 1] - 48 : content.value[2 * i + 1] - 55);
    }

    return message;
}

/*!
 * @brief: according to the flag of the type of the previous code and the flag of the type of the current code to determine the prefix
 * @param flag_pre_code: the flag of the type of the previous code
 * @param flag_cur_code: the flag of the type of the current code
 * @return: the prefix in unsigned char
 */
unsigned char get_prefix(unsigned int flag_pre_code, unsigned int flag_cur_code)
{
    /*
     * flag: value --- meaning
     *          -1 --- initial state
     *           0 --- ordinal
     *           1 --- codeword
     *           2 --- string-extension length
     *           3 --- control code
     */
    if (flag_pre_code == 1)
    {
        if (flag_cur_code == 0)
        {
            return 3; // 00
        }
        else if (flag_cur_code == 2)
        {
            return 2; // 10
        }
        else
        {
            return 1; // 1
        }
    }
    else
    {
        if (flag_cur_code == 0)
        {
            return 0; // 0
        }
        else
        {
            return 1; // 1
        }
    }
}

/*!
 * @brief: transform the prefix to bit stream
 * @param prefix: the prefix to be handled
 * @param struct_return: the struct to save the bit stream
 */
void handle_prefix(unsigned char prefix, STRUCTRETURN *struct_return)
{
    // according to the value of return of the function named get_prefix to set the prefix to bit stream
    if (prefix < 2)
    {
        // the prefix is '0' or '1'
        struct_return->value = (unsigned char *) calloc(1, sizeof(unsigned char));
        struct_return->value[0] = prefix;

        struct_return->len = 1;
    }
    else
    {
        // the prefix is '00'(the value of return of the function named get_prefix is three(3), because the prefix '0' is the zero(0)) or '10'
        struct_return->value = (unsigned char *) calloc(2, sizeof(unsigned char));
        struct_return->value[0] = 0;
        struct_return->value[1] = prefix == 2 ? 1 : 0;

        struct_return->len = 2;
    }
}

/*!
 * @brief: handle a new character
 * @param cur_char: the new character
 * @param self: the struct to save data
 */
void new_character(unsigned char cur_char, CSELF *self)
{
    // if the new character is not the last character of message
    if (self->index_message < (unsigned int) self->len_message - 1)
    {
        // add the next character of the new character to the root_array[cur_char]'s child
        NODE *tmp_node = (NODE *) calloc(1, sizeof(NODE));
        init_node(tmp_node, self->params[1][0], self->index_message, 1);
        add_child(&self->array_node.arr_node[cur_char], tmp_node, &self->array_node);

        // update the params[1][0]
        self->params[1][0]++;

        free(tmp_node);
        tmp_node = NULL;
    }
}

/*!
 * @brief: set a code in unsigned int to bit stream
 * @param code: the code to be handled
 * @param len: the length of the bit stream, if the length of code in bit is less than the length that the function should set, the function should pad it with zero(0)
 * @param struct_return: the struct to save bit stream
 */
void handle_code(unsigned int code, unsigned char len, STRUCTRETURN *struct_return)
{
    struct_return->len = len;
    struct_return->value = (unsigned char *) calloc(len, sizeof(unsigned char));
    // pay attention to the order
    for (int i = 0; i < len; ++i)
    {
        struct_return->value[len - 1 - i] = code >> (len - 1 - i) & 0x01;
    }
}

/*!
 * @brief: the string-extension procedure
 * @param self: the struct to save data
 * @param node_last_matched: the node that is the last node matched while matching the longest string
 * @return: the string-extension length
 */
unsigned int extend_string_segment(CSELF *self, NODE *node_last_matched)
{
    unsigned int len_string_seg = 0;
    unsigned int pos_history = (unsigned int) node_last_matched->first_char_pos + node_last_matched->seg_length;
    // compare the new character with decode history one by one
    while (self->message[self->index_message] == self->message[pos_history])
    {
        len_string_seg++;
        pos_history++;
        self->index_message++;
        // the message has been traversed to the end or the string-extension length is up to 253
        if ((unsigned int) self->index_message == self->len_message || len_string_seg == 254)
        {
            break;
        }
    }

    NODE *tmp_node = (NODE *) calloc(1, sizeof(NODE));
    // if the string has been extended
    if (len_string_seg != 0)
    {
        init_node(tmp_node, self->params[1][0], (unsigned int) self->index_message - len_string_seg, len_string_seg);
        add_child(node_last_matched, tmp_node, &self->array_node);
    }
    else
    {
        // add the next character to the list of children of the node last matched
        init_node(tmp_node, self->params[1][0], self->index_message, 1);
        add_child(node_last_matched, tmp_node, &self->array_node);
    }
    self->params[1][0]++;

    free(tmp_node);
    tmp_node = NULL;

    return len_string_seg;
}

/*!
 * @brief: transform the string-extension length to bit stream
 * @param string_extension_length: the string-extension length to be transformed
 * @param n7: the param that determines the length of the bit stream while the string-extension length is greater than 12
 * @param struct_return: the struct to save the bit stream
 */
void get_string_extension_length_subfields(unsigned char string_extension_length, unsigned int n7,
                                           STRUCTRETURN *struct_return)
{
    struct_return->len = 0;
    struct_return->value = NULL;
    if (string_extension_length == 1) // the string-extension length is equal to one(1)
    {
        struct_return->value = (unsigned char *) calloc(1, sizeof(unsigned char));
        struct_return->value[0] = 1;
        struct_return->len = 1;
    }
    else if (1 < string_extension_length && string_extension_length <= 4) // the string-extension length is in [2,4]
    {
        struct_return->value = (unsigned char *) calloc(3, sizeof(unsigned char));
        struct_return->value[0] = 0;
        for (int i = 0; i < 2; ++i)
        {
            struct_return->value[2 - i] = (string_extension_length - 1) >> (1 - i) & 0x01;
        }
        struct_return->len = 3;
    }
    else if (4 < string_extension_length && string_extension_length <= 12) // the string-extension length is in [5,12]
    {
        struct_return->value = (unsigned char *) calloc(7, sizeof(unsigned char));
        for (int i = 0; i < 4; ++i)
        {
            struct_return->value[i] = 0;
        }
        for (int i = 0; i < 3; ++i)
        {
            struct_return->value[6 - i] = (string_extension_length - 5) >> (2 - i) & 0x01;
        }
        struct_return->len = 7;
    }
    else // the string-extension length is in [13,255]
    {
        // according to the N7 to know how many bits were cost on representing the string-extension length
        unsigned int align = 5, total = 9;
        if (32 <= n7 && n7 <= 46)
        {
            align = 5;
            total = 9;
        }
        else if (46 < n7 && n7 <= 78)
        {
            align = 6;
            total = 10;
        }
        else if (78 < n7 && n7 <= 142)
        {
            align = 7;
            total = 11;
        }
        else if (142 < n7 && n7 <= 255)
        {
            align = 8;
            total = 12;
        }

        struct_return->value = (unsigned char *) calloc(total, sizeof(unsigned char));
        for (int i = 0; i < 3; ++i)
        {
            struct_return->value[i] = 0;
        }
        struct_return->value[3] = 1;
        for (int i = 0; i < align; ++i) // NOLINT(cppcoreguidelines-narrowing-conversions)
        {
            struct_return->value[total - 1 - i] = (string_extension_length - 13) >> (align - 1 - i) & 0x01;
        }
        struct_return->len = total;
    }
}

/*!
 * @brief: update the result of compress with the code and the type of the code
 * @param result_compress: the struct to save result
 * @param self: the struct to save data
 * @param code: the code to be handled
 * @param type: the type of the code
 */
void update_result_compress(STRUCTRESULT *result_compress, CSELF *self, unsigned int code, unsigned int type)
{
    /*
     * according to the type of the code, the procedure will take different action
     * type: the type of the code
     *  value --- meaning
     *      0 --- prefix
     *      1 --- ordinal
     *      2 --- code (including the codeword and the control code)
     *      3 --- string-extension length
     */
    STRUCTRETURN struct_return = {0, NULL};
    if (type == 0) // handle a prefix
    {
        unsigned int prefix = get_prefix(self->flag_pre_code, code);
        handle_prefix(prefix, &struct_return);
    }
    else if (type == 1)// handle an ordinal
    {
        handle_code(code, self->params[1][4], &struct_return);
    }
    else if (type == 2) // handle a codeword or a control code
    {
        handle_code(code, self->params[1][1], &struct_return);
    }
    else if (type == 3) // handle a string-extension length
    {
        get_string_extension_length_subfields(code, self->params[0][6], &struct_return);
    }

    // update the result
    memcpy(result_compress->result + result_compress->len, struct_return.value, struct_return.len);
    result_compress->len += struct_return.len;

    free(struct_return.value);
    struct_return.value = NULL;
}

/*!
 * @brief: write the file with the struct of result of compress
 * @param result_compress: the struct of result of compress
 */
void write_compress_result_file(STRUCTRESULT result_compress)
{
    // Padding zero(0) to establish the octet-alignment
    result_compress.len = result_compress.len % 8 == 0 ? result_compress.len : 8 * (result_compress.len / 8 + 1);

    FILE *fp = fopen("messageCompressResult.txt", "w");
    if (fp == NULL)
    {
        perror("Fopen(messageCompressResult.txt) error:");

        return;
    }

    // write to file octet-alignment by octet-alignment
    // it's need to be noted that the file(V.44 Data Compression Procedures) has stated that the lower four bits should be written before the higher four bits
    unsigned int round = result_compress.len / 8;
    for (int i = 0; i < round; ++i)
    {
        fprintf(fp, "%X", (result_compress.result[8 * i + 7] << 3) + (result_compress.result[8 * i + 6] << 2) +
                          (result_compress.result[8 * i + 5] << 1) + (result_compress.result[8 * i + 4] << 0));
        fprintf(fp, "%X", (result_compress.result[8 * i + 3] << 3) + (result_compress.result[8 * i + 2] << 2) +
                          (result_compress.result[8 * i + 1] << 1) + (result_compress.result[8 * i + 0] << 0));
    }

    fclose(fp);
    fp = NULL;
}
