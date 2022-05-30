# encoding = utf-8

class Node:
    def __init__(self, elem, data, child_list):
        """
        :param elem: node value, type: int
        :param data: a dict, key-value: codeword:(int) first_char_pos:(int) seg_length:(int)
                                        down_index:(int) side_index:(int)
        :param child_list: a list of children
        """
        self.elem = elem
        self.data = data
        self.children = child_list

    def add(self, child_node):
        self.children.append(child_node)

    def match_child_node_elem_by_length(self, possible_longest_string):
        for child in self.children:
            elem_child = child.elem
            len_elem_child = int(len(elem_child) / 8)
            if len_elem_child <= len(possible_longest_string):
                if elem_child == ''.join(possible_longest_string[:len_elem_child]):
                    return child, len_elem_child
        return None, 0


class Tree:
    def __init__(self, root_node):
        self.root_node = root_node

    def match_node_with_longest_string(self, possible_longest_string):
        """
        对可能的最长字符串与树节点进行匹配，支持节点元素不定长匹配
        :param possible_longest_string:可能的最长字符串
        """
        cur_node = self.root_node
        len_total_matched = 0
        if cur_node.elem == possible_longest_string[0]:
            len_total_matched += 1
        child_node, len_last_matched = cur_node.match_child_node_elem_by_length(possible_longest_string[1:])
        yield child_node, len_total_matched
        while child_node is not None:
            len_total_matched += len_last_matched
            child_node, len_last_matched = child_node.match_child_node_elem_by_length(
                possible_longest_string[len_total_matched:])
            yield child_node, len_total_matched


class LZJHEncoder:
    def __init__(self, params, message):
        self.params = params
        # index means the position in encoder history, value indicates the input character
        self.history = []
        # index from 0 to (N4)-1 means the root
        self.root_array = [Tree(Node('{:0>8}'.format(bin(i)[2:]),
                                     {'codeword': 0, 'first_char_pos': 0, 'seg_length': 1, 'down_index': 0,
                                      'side_index': 0}, [])) for i in range(256)]
        self.message = message
        self.len_message = len(message)
        self.index_message = 0

    def encode(self):
        self.len_message = len(self.message)
        self.index_message = 0
        self.params[1][0] = 4
        while self.index_message < self.len_message:
            cur_char = self.message[self.index_message]
            ordinal_cur_char = int(cur_char, 2)

            # 更新encode history
            self.history.append(cur_char)
            self.index_message += 1
            if self.root_array[ordinal_cur_char].root_node.data['down_index'] == 0:  # new character
                self.new_character(ordinal_cur_char)
                yield 'ordinal' + cur_char, "{0:0>{1}}".format(bin(ordinal_cur_char)[2:], self.params[1][4])
            else:
                # 最长字符串匹配
                list_match_result = []
                generator_match_result = self.match_longest_string(self.message[self.index_message - 1:])
                for match_result in generator_match_result:  # 提取生成器数据
                    list_match_result.append(match_result)
                len_longest_string_matched = list_match_result[-1][1]

                # 更新encode history
                for m in self.message[self.index_message:self.index_message + len_longest_string_matched - 1]:
                    self.history.append(m)
                    self.index_message += 1
                # 判断是否进入字符串拓展
                if len_longest_string_matched == 1:  # 只匹配了根节点，相应子节点未出现过，不进入拓展
                    yield 'ordinal' + self.message[self.index_message - 1], "{0:0>{1}}".format(
                        bin(int(self.message[self.index_message - 1], 2))[2:], self.params[1][4])
                    if self.index_message < self.len_message:
                        # 将该节点加入子节点列表
                        tmp_node_data = {'codeword': self.params[1][0], 'first_char_pos': self.index_message,
                                         'seg_length': 1, 'down_index': 0, 'side_index': 0}
                        tmp_node_children_list = []
                        tmp_node = Node(self.message[self.index_message], tmp_node_data, tmp_node_children_list)
                        for root_child in self.root_array[int(self.message[self.index_message - 1], 2)] \
                                .root_node.children:
                            root_child.data['side_index'] = self.params[1][0]
                        self.root_array[int(self.message[self.index_message - 1], 2)].root_node.add(tmp_node)
                        self.params[1][0] += 1
                        self.check_codeword_size()
                else:
                    yield 'codeword' + str(
                        int("{0:0>{1}}".format(bin(list_match_result[-2][0].data['codeword'])[2:], self.params[1][1]),
                            2)), "{0:0>{1}}".format(bin(list_match_result[-2][0].data['codeword'])[2:],
                                                    self.params[1][1])
                    if self.index_message < self.len_message:
                        # 字符串拓展
                        tup_extend_result = self.extend_string_segment(list_match_result)
                        if tup_extend_result[1] != 0:
                            yield 'stringExtensionLength: ' + str(
                                tup_extend_result[1]), tup_extend_result[1]

    def new_character(self, ordinal_cur_char):
        # 更新root array
        self.root_array[ordinal_cur_char].root_node.data['down_index'] = self.params[1][0]

        # 下一个输入追加到根节点之后
        if self.index_message < self.len_message - 1:
            self.root_array[ordinal_cur_char].root_node.add(Node(self.message[self.index_message],
                                                                 {'codeword': self.params[1][0],
                                                                  'first_char_pos': self.index_message, 'seg_length': 1,
                                                                  'down_index': 0, 'side_index': 0}, []))
            self.params[1][0] += 1
            self.check_codeword_size()

    def match_longest_string(self, longest_string):
        return self.root_array[int(longest_string[0], 2)].match_node_with_longest_string(longest_string)

    def extend_string_segment(self, list_match_result):
        last_node_matched = list_match_result[-2][0]  # 最后匹配到的节点
        history_pos = last_node_matched.data['first_char_pos'] + last_node_matched.data['seg_length']
        len_string_seg = 0
        while self.history[history_pos] == self.message[self.index_message]:
            # 更新encode history
            self.history.append(self.message[self.index_message])
            history_pos += 1
            len_string_seg += 1
            self.index_message += 1
            if self.index_message == self.len_message:
                break

        last_node_matched.data['down_index'] = self.params[1][0]
        if len_string_seg != 0:  # 拓展了部分字符串
            # 将拓展的字符串加入到最后匹配到的节点的子节点列表中
            tmp_node = Node(''.join(self.message[history_pos - len_string_seg:history_pos]),
                            {'codeword': self.params[1][0], 'first_char_pos': self.index_message - len_string_seg,
                             'seg_length': len_string_seg, 'down_index': 0, 'side_index': 0}, [])
            last_node_matched.add(tmp_node)
        else:  # 未拓展
            next_char_node = Node(self.message[self.index_message],
                                  {'codeword': self.params[1][0], 'first_char_pos': self.index_message,
                                   'seg_length': 1, 'down_index': 0, 'side_index': 0}, [])
            last_node_matched.add(next_char_node)
        self.params[1][0] += 1
        self.check_codeword_size()
        return self.message[history_pos - len_string_seg:history_pos], len_string_seg

    def check_codeword_size(self):
        if self.params[1][0] == 128:
            self.params[1][1] = 9


class LZJHDecoder:
    def __init__(self, params, list_encode_result):
        self.params = params
        self.list_encode_result = list_encode_result
        self.index_history = 0
        self.history = []
        self.list_string_collection = []
        self.params[1][0] = 4

    def decode(self):
        # 遍历编码结果，恢复原始数据
        flag_pre_code = -1  # -1---(Init REINIT ECM) 0---ordinal 1---codeword 2---string extension length
        for index_list_encode_result, encode_result in enumerate(self.list_encode_result):
            if type(encode_result[1]) == str:  # ordinal or codeword
                if len(encode_result[1]) == self.params[1][4]:  # ordinal
                    self.history.append(encode_result[1])
                    self.index_history += 1
                    if flag_pre_code == 0:  # previous code is also an ordinal
                        self.list_string_collection.append(self.new_string_collection(self.index_history - 1, 2))
                    elif flag_pre_code == 1:  # previous code is a codeword
                        pre_codeword = int(self.list_encode_result[index_list_encode_result - 1][1], 2)
                        pre_string_collection = self.search_string_collection_by_codeword(pre_codeword)
                        length_pre_string = pre_string_collection['string_length']
                        self.list_string_collection.append(
                            self.new_string_collection(self.index_history - 1, length_pre_string + 1))
                    elif flag_pre_code == 2:  # previous code is a string-extension length
                        pass
                    elif flag_pre_code == -1:  # the current code is the first code
                        pass
                    flag_pre_code = 0
                elif len(encode_result[1]) != self.params[1][4]:  # codeword
                    cur_codeword = int(encode_result[1], 2)
                    if cur_codeword < self.params[1][0]:  # the codeword received is less than C1
                        string_collection_of_codeword = self.search_string_collection_by_codeword(cur_codeword)
                        length_cur_string = string_collection_of_codeword['string_length']
                        last_char_pos_pre_string = string_collection_of_codeword['last_char_pos']
                        first_char_pos_pre_string = 1 + last_char_pos_pre_string - length_cur_string
                        for i in range(length_cur_string):
                            self.history.append(self.history[first_char_pos_pre_string + i])
                            self.index_history += 1
                        if flag_pre_code == 0:
                            self.list_string_collection.append(
                                self.new_string_collection(self.index_history - length_cur_string, 2))
                        elif flag_pre_code == 1:
                            codeword_of_pre_string = int(self.list_encode_result[index_list_encode_result - 1][1], 2)
                            string_collection_of_pre_codeword = self.search_string_collection_by_codeword(
                                codeword_of_pre_string)
                            length_pre_string = string_collection_of_pre_codeword['string_length']
                            self.list_string_collection.append(
                                self.new_string_collection(self.index_history - length_cur_string,
                                                           length_pre_string + 1))
                        elif flag_pre_code == 2:
                            pass
                    elif cur_codeword == self.params[1][0]:
                        if flag_pre_code == 0:  # previous encode result is an ordinal
                            pre_char = self.list_encode_result[index_list_encode_result - 1][1]
                            self.history.append(pre_char)
                            self.index_history += 1
                            self.history.append(pre_char)
                            self.index_history += 1
                            self.list_string_collection.append(self.new_string_collection(self.index_history - 2, 2))
                        elif flag_pre_code == 1:  # previous encode result is a codeword
                            string_collection_of_pre_codeword = self.search_string_collection_by_codeword(
                                int(self.list_encode_result[index_list_encode_result - 1][1], 2))
                            length_pre_string = string_collection_of_pre_codeword['string_length']
                            last_char_pos_pre_string = string_collection_of_pre_codeword['last_char_pos']
                            first_char_pos_pre_string = 1 + last_char_pos_pre_string - length_pre_string
                            for i in range(length_pre_string):
                                self.history.append(self.history[first_char_pos_pre_string + i])
                                self.index_history += 1
                            self.history.append(self.history[first_char_pos_pre_string])
                            self.index_history += 1
                            self.list_string_collection.append(
                                self.new_string_collection(self.index_history - 2, length_pre_string + 1))
                    flag_pre_code = 1
            else:  # string extension length, only after codeword
                string_seg_length = encode_result[1]
                pre_codeword = int(self.list_encode_result[index_list_encode_result - 1][1], 2)
                pre_string_collection = self.search_string_collection_by_codeword(pre_codeword)
                last_char_pos_pre_string = pre_string_collection['last_char_pos']
                length_pre_string = pre_string_collection['string_length']
                while string_seg_length > 0:
                    self.history.append(self.history[last_char_pos_pre_string + 1])
                    last_char_pos_pre_string += 1
                    self.index_history += 1
                    string_seg_length -= 1
                self.list_string_collection.append(self.new_string_collection(self.index_history - 1,
                                                                              encode_result[1] + length_pre_string))
                flag_pre_code = 2
        return self.history

    def new_string_collection(self, last_char_pos, string_length):
        tmp_dict = {'codeword': self.params[1][0], 'last_char_pos': last_char_pos, 'string_length': string_length}
        # print(tmp_dict)
        self.params[1][0] += 1
        if self.params[1][0] == 128:  # 超出7比特可表示范围，拓展到9比特
            self.params[1][1] = 9

        return tmp_dict

    def search_string_collection_by_codeword(self, codeword_searched):
        for string_collection in self.list_string_collection:
            if string_collection['codeword'] == codeword_searched:
                return string_collection
