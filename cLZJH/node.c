//
// Created by ngr02 on 2021/6/10.
//
#include "node.h"
#include <string.h>

/*!
 * @brief: Init a node, the down_index and the side_index will be set default value(0)
 * @param initialized_node: the node to be initialized
 * @param codeword: the codeword of the node
 * @param first_char_pos: the first character of the string segment position in the message of the node
 * @param seg_length: the length of the string segment of the node
 */
void init_node(NODE *initialized_node, unsigned int codeword, unsigned int first_char_pos, unsigned int seg_length)
{
    initialized_node->codeword = codeword;
    initialized_node->first_char_pos = first_char_pos;
    initialized_node->seg_length = seg_length;
    initialized_node->down_index = 0;
    initialized_node->side_index = 0;
}

unsigned static int get_last_index_child(unsigned int index_child, ARRAYNODE *array_node)
{
    while (array_node->arr_node[index_child].side_index != (unsigned int) 0)
    {
        index_child = array_node->arr_node[index_child].side_index;
    }

    return index_child;
}

/*!
 * @brief: Add a child node for the parent node
 * @param parent: the parent node
 * @param child: the child node
 * @param array_node: the array of the nodes
 */
void add_child(NODE *parent, NODE *child, ARRAYNODE *array_node)
{
    if (parent->down_index == (unsigned int) 0) // the parent does not have any children
    {
        // set the down_index of the parent node with the value of array_node->count_node
        parent->down_index = array_node->count_node;
    }
    else
    {
        // get the last child node and set the side_index of the value of array_node->count_node
        unsigned int final_side_index = get_last_index_child(parent->down_index, array_node);
        array_node->arr_node[final_side_index].side_index = array_node->count_node;
    }

    // copy the child node to be added to the array_node->arr_node[array_node->count_node] to save the child node
    memcpy(&array_node->arr_node[array_node->count_node], child, sizeof(NODE));
    array_node->count_node++;
}


/*!
 * @brief: match a node by comparing the elem of the node with the part of the message starting from the index
 * @param message: the unsigned char string of the message
 * @param index_message: the index of the message
 * @param node_match: the node to be matched
 * @param len_message: the length of the unsigned char string of the message
 * @return: if the match is successful, the length of the elem will be returned; otherwise, zero(0) will be returned
 */
static unsigned int
match_a_node(const unsigned char *message, unsigned int index_message, NODE *node_match, unsigned int len_message)
{
    unsigned int len_string_matched = 0;
    size_t len_cur_elem = node_match->seg_length;

    // traverse the elem of the node to match
    while (len_cur_elem > 0)
    {
        if (message[index_message] == *(message + (int) node_match->first_char_pos + len_string_matched))
        {
            len_string_matched++;
            index_message++;
            len_cur_elem--;

            // the unsigned character matched is the last one of the string of the message
            if (index_message == len_message)
            {
                // if the elem of the node still have the rest
                if (len_cur_elem > 0)
                {
                    // the match should be terminated and return the zero(0)
                    len_string_matched = 0;
                }
                break;
            }
        }
        else
        {
            /*
             *  once the differences between the elem of the node and the message are found
             *  the match should be terminated and return the zero(0)
             */
            len_string_matched = 0;
            break;
        }
    }
    return len_string_matched;
}

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
                          ARRAYNODE *array_node, unsigned int len_message)
{
    NODE *node_match = &array_node->arr_node[array_node->arr_node[message[index_message - 1]].down_index];

    // search the node of the array_node by the index of it until the node is not initialized
    while ((unsigned int) node_match->seg_length != 0)
    {
        // match current node
        unsigned int count = match_a_node(message, index_message, node_match, len_message);
        if (count == 0)// the node does not match the possible string and try its next brother node
        {
            // if the node has next brother node, set the node_match with the next brother node
            if ((unsigned int) node_match->side_index != 0)
            {
                node_match = &array_node->arr_node[node_match->side_index];
            }
            else
            {
                return;
            }
        }
        else // the node do match the possible string, push the index_message back a few bits matched
        {
            /*
             * if the index_message after being plussed the result of current match is greater than the length of the message
             * the result of current match should not be recorded and the procedure should return immediately
            */
            if (index_message + count > len_message)
            {
                return;
            }
                /*
                 * the index_message after being plussed the result of current match is equal to the length of the message
                 * the result of current match should not be recorded and the match should not continue
                */
            else if (index_message + count == len_message)
            {
                final_result->len_matched += count;
                final_result->node_matched = node_match;

                return;
            }
            else
            {
                index_message += count;
                final_result->len_matched += count;
                final_result->node_matched = node_match;

                /*
                 * if the current node do have at least one child, the match should continue with its child
                 */
                if ((unsigned int) node_match->down_index != 0)
                {
                    node_match = &array_node->arr_node[node_match->down_index];
                }
                else
                {
                    return;
                }
            }
        }
    }
}