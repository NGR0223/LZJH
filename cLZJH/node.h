//
// Created by ngr02 on 2021/6/10.
//

#ifndef CLZJH_NODE_H
#define CLZJH_NODE_H

/*!
 * @brief: The struct of node, the member of it is defined according the file (V.44 Data Compression Procedures)
 */
typedef struct node
{
    unsigned int codeword: 10;
    unsigned int first_char_pos: 14;
    unsigned int seg_length: 8;
    unsigned int down_index: 16;
    unsigned int side_index: 16;
} NODE;

/*!
 * @brief: The struct of an array of nodes, the meaning of its member is as follows:
 *         count_node: the next available index of the array of the nodes
 *         arr_node: the array of the nodes where saves the nodes
 */
typedef struct array_node
{
    unsigned int count_node;
    NODE arr_node[1024];
} ARRAYNODE;

/*!
 * @brief: The struct of the result of matching longest string, the meaning of its member is as follows:
 *         len_matched: the length of the longest string matched
 *         node_matched: the last node matched
 */
typedef struct match_result
{
    unsigned char len_matched;
    NODE *node_matched;
} MATCHRESULT;

/*!
 * @brief: Init a node, the down_index and the side_index will be set default value(0)
 * @param initialized_node: the node to be initialized
 * @param codeword: the codeword of the node
 * @param first_char_pos: the first character of the string segment position in the message of the node
 * @param seg_length: the length of the string segment of the node
 */
void init_node(NODE *initialized_node, unsigned int codeword, unsigned int first_char_pos, unsigned int seg_length);

/*!
 * @brief: Add a child node for the parent node
 * @param parent: the parent node
 * @param child: the child node
 * @param array_node: the array of the nodes
 */
void add_child(NODE *parent, NODE *child, ARRAYNODE *array_node);

/*!
 * @brief: Match the longest string from a character of the message by comparing the message after the character
 *         with the node in root_array whose elem is same as the character
 * @param message: the string of the message
 * @param index_message: the index of the message
 * @param final_result: the final result of the matching longest string
 * @param array_node: the array of the nodes
 * @param len_message: the length of the message
 */
void match_longest_string(const unsigned char *message, unsigned int index_message, MATCHRESULT *final_result,
                          ARRAYNODE *array_node, unsigned int len_message);

#endif //CLZJH_NODE_H
