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
        self.flag_pre_code = -1

    def encode(self):
        while self.index_message < self.len_message:
            cur_char = self.message[self.index_message]
            ordinal_cur_char = int(cur_char, 2)

            # 更新encode history
            self.history.append(cur_char)
            self.index_message += 1
            if self.root_array[ordinal_cur_char].root_node.data['down_index'] == 0:  # new character
                if len(bin(ordinal_cur_char)) > self.params[1][4] + 2:  # 判断当前序数是否超出C5
                    self.params[1][4] += 1
                    yield 'controlCodePrefix', self.get_prefix(3)
                    yield 'ordinalSetup', "{0:0>{1}}".format(bin(2)[2:], self.params[1][1])
                    self.flag_pre_code = 3
                self.new_character(ordinal_cur_char)
                yield 'ordinalPrefix', self.get_prefix(0)
                yield 'ordinal' + cur_char, "{0:0>{1}}".format(bin(ordinal_cur_char)[2:], self.params[1][4])
                self.flag_pre_code = 0
            else:
                # the longest string matching
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
                    if len(bin(int(self.message[self.index_message - 1], 2))) > self.params[1][4] + 2:  # 判断当前序数是否超出C5
                        self.params[1][4] += 1
                        yield 'controlCodePrefix', self.get_prefix(3)
                        yield 'ordinalSetup', "{0:0>{1}}".format(bin(2)[2:], self.params[1][1])
                        self.flag_pre_code = 3
                    yield 'ordinalPrefix', self.get_prefix(0)
                    yield 'ordinal' + self.message[self.index_message - 1], "{0:0>{1}}".format(
                        bin(int(self.message[self.index_message - 1], 2))[2:], self.params[1][4])
                    self.flag_pre_code = 0
                    if self.index_message < self.len_message:
                        # 将下一字节作为节点加入子节点列表
                        tmp_node_data = {'codeword': self.params[1][0], 'first_char_pos': self.index_message,
                                         'seg_length': 1, 'down_index': 0, 'side_index': 0}
                        tmp_node = Node(self.message[self.index_message], tmp_node_data, [])
                        for root_child in self.root_array[int(self.message[self.index_message - 1], 2)] \
                                .root_node.children:
                            root_child.data['side_index'] = self.params[1][0]
                        self.root_array[int(self.message[self.index_message - 1], 2)].root_node.add(tmp_node)
                        self.params[1][0] += 1
                else:
                    # 检查当前codeword二进制长度是否大于C2
                    while len(bin(list_match_result[-2][0].data['codeword'])) > self.params[1][1] + 2:
                        yield 'controlCodePrefix', self.get_prefix(3)
                        yield 'codewordSetup', "{0:0>{1}}".format(bin(2)[2:], self.params[1][1])
                        self.params[1][1] += 1
                        self.params[1][2] *= 2
                        self.flag_pre_code = 3
                    cur_codeword = "{0:0>{1}}".format(bin(list_match_result[-2][0].data['codeword'])[2:],
                                                      self.params[1][1])
                    yield 'codewordPrefix', self.get_prefix(1)
                    yield 'codeword' + str(int(cur_codeword, 2)), cur_codeword
                    self.flag_pre_code = 1
                    if self.index_message < self.len_message:
                        # 字符串拓展
                        tup_extend_result = self.extend_string_segment(list_match_result)
                        if tup_extend_result[1] != 0:
                            yield 'stringExtensionLengthPrefix', self.get_prefix(2)
                            yield 'stringExtensionLength: ' + str(
                                tup_extend_result[1]), self.get_string_extension_length_subfields(tup_extend_result[1])
                            self.flag_pre_code = 2
        yield 'controlCodePrefix', self.get_prefix(3)
        yield 'FLUSH', "{0:0>{1}}".format(bin(1)[2:], self.params[1][1])
        self.flag_pre_code = 3

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
            if self.index_message == self.len_message or len_string_seg == 254:
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
        return self.message[history_pos - len_string_seg:history_pos], len_string_seg

    def get_prefix(self, flag_cur_code):
        """
                    :flag_pre_code: a flag indicates previous code's type
                    :flag_cur_code: a flag indicates current code's type
                    -1 --- initial state
                     0 --- ordinal
                     1 --- codeword
                     2 --- string-extension length
                     3 --- control code
        """
        if self.flag_pre_code == 1:
            if flag_cur_code == 0:
                return '00'
            elif flag_cur_code == 2:
                return '10'
            else:
                return '1'
        else:
            if flag_cur_code == 0:
                return '0'
            else:
                return '1'

    def get_string_extension_length_subfields(self, string_extension_length):
        if string_extension_length == 1:
            return '1'
        elif 1 < string_extension_length <= 4:
            return '0', '{:0>2}'.format(bin(string_extension_length - 1)[2:])
        elif 4 < string_extension_length <= 12:
            return '0', '00', '0', '{:0>3}'.format(bin(string_extension_length - 5)[2:])
        else:
            if 32 <= self.params[0][6] <= 46:
                return '0', '00', '1', '{:0>5}'.format(bin(string_extension_length - 13)[2:])
            elif 46 < self.params[0][6] <= 78:
                return '0', '00', '1', '{:0>6}'.format(bin(string_extension_length - 13)[2:])
            elif 78 < self.params[0][6] <= 142:
                return '0', '00', '1', '{:0>7}'.format(bin(string_extension_length - 13)[2:])
            elif 142 < self.params[0][6] <= 255:
                return '0', '00', '1', '{:0>8}'.format(bin(string_extension_length - 13)[2:])


class LZJHDecoder:
    def __init__(self, params, string_message_compressed):
        self.params = params
        self.string_message = string_message_compressed
        self.len_string_message = len(string_message_compressed)
        self.index_string_message = self.len_string_message - 1
        self.index_history = 0
        self.history = []
        self.list_string_collection = []
        self.flag_pre_code = -1  # -1---(Init REINIT ECM) 0---ordinal 1---codeword 2---string-extension length
        self.flag_first_two_code = -1

    # noinspection DuplicatedCode
    def decode(self):
        # 遍历已压缩文件的比特串
        while self.index_string_message >= 0:
            if self.index_string_message == self.len_string_message - 1:
                if self.string_message[self.index_string_message] == '0':  # an ordinal
                    self.handle_ordinal()
                    self.flag_pre_code = 0
                elif self.string_message[self.index_string_message] == '1':  # a codeword or a control code
                    int_cur_code = int(
                        self.string_message[self.index_string_message - self.params[1][1]:self.index_string_message], 2)
                    self.index_string_message -= self.params[1][1] + 1
                    if int_cur_code < 4:  # current code is a control code
                        self.handle_control_code(int_cur_code)
                        self.flag_first_two_code = self.flag_first_two_code if self.flag_pre_code > 2 \
                            else self.flag_pre_code
                        self.flag_pre_code = 3 if int_cur_code == 1 else 4
                    else:
                        self.handle_codeword(int_cur_code)
                        self.flag_pre_code = 1
            else:
                if self.flag_pre_code == 1:  # previous code is a codeword
                    # current code is a codeword or a control code
                    if self.string_message[self.index_string_message] == '1':
                        int_cur_code = int(self.string_message[
                                           self.index_string_message - self.params[1][1]:self.index_string_message], 2)
                        self.index_string_message -= self.params[1][1] + 1
                        if int_cur_code < 4:  # current code is a control code
                            self.handle_control_code(int_cur_code)
                            self.flag_first_two_code = self.flag_first_two_code if self.flag_pre_code > 2 \
                                else self.flag_pre_code
                            self.flag_pre_code = 3 if int_cur_code == 1 else 4
                        else:
                            self.handle_codeword(int_cur_code)
                            self.flag_pre_code = 1
                    else:
                        self.index_string_message -= 1
                        # current code is a string-extension length
                        if self.string_message[self.index_string_message] == '1':
                            self.handle_string_extension_length()
                            self.flag_pre_code = 2
                        else:  # current code is an ordinal
                            self.handle_ordinal()
                            self.flag_pre_code = 0
                elif self.flag_pre_code == 3:
                    if self.string_message[self.index_string_message] == '1':
                        int_cur_code = int(self.string_message[
                                           self.index_string_message - self.params[1][1]:self.index_string_message], 2)
                        self.index_string_message -= self.params[1][1] + 1
                        self.handle_codeword(int_cur_code)
                        self.flag_pre_code = 1
                    else:
                        self.handle_ordinal()
                        self.flag_pre_code = 0
                else:  # previous code is others codes
                    # current code is a codeword or a control code
                    if self.string_message[self.index_string_message] == '1':
                        int_cur_code = int(self.string_message[
                                           self.index_string_message - self.params[1][1]:self.index_string_message], 2)
                        self.index_string_message -= self.params[1][1] + 1
                        if int_cur_code < 4:  # current code is a control code
                            self.handle_control_code(int_cur_code)
                            self.flag_first_two_code = self.flag_first_two_code if self.flag_pre_code > 2 \
                                else self.flag_pre_code
                            self.flag_pre_code = 3 if int_cur_code == 1 else 4
                        else:
                            self.handle_codeword(int_cur_code)
                            self.flag_pre_code = 1
                    else:  # current code is an ordinal
                        self.handle_ordinal()
                        self.flag_pre_code = 0
        return self.history

    def new_string_collection(self, last_char_pos, string_length):
        tmp_dict = {'codeword': self.params[1][0], 'last_char_pos': last_char_pos, 'string_length': string_length}
        self.params[1][0] += 1

        return tmp_dict

    def search_string_collection_by_codeword(self, codeword_searched):
        for string_collection in self.list_string_collection:
            if string_collection['codeword'] == codeword_searched:
                return string_collection

    def transfer_string_extension_length(self):
        self.index_string_message -= 1
        if self.string_message[self.index_string_message] == '1':  # length = 1
            return 1, 1
        else:
            self.index_string_message -= 2
            if self.string_message[self.index_string_message:self.index_string_message + 2] != '00':  # 2 <= length <= 4
                return int(self.string_message[self.index_string_message:self.index_string_message + 2], 2) + 1, 3
            else:
                self.index_string_message -= 4
                if self.string_message[self.index_string_message + 3] == '0':  # 5 <= length <= 12
                    return int(self.string_message[self.index_string_message:self.index_string_message + 3], 2) + 5, 7
                else:  # length >= 13, according N7T to read the bits
                    if 32 <= self.params[0][6] <= 46:
                        self.index_string_message -= 2
                        return int(self.string_message[self.index_string_message:self.index_string_message + 5],
                                   2) + 13, 9
                    elif 46 < self.params[0][6] <= 78:
                        self.index_string_message -= 3
                        return int(self.string_message[self.index_string_message:self.index_string_message + 6],
                                   2) + 13, 10
                    elif 78 < self.params[0][6] <= 142:
                        self.index_string_message -= 4
                        return int(self.string_message[self.index_string_message:self.index_string_message + 7],
                                   2) + 13, 11
                    elif 142 < self.params[0][6] <= 255:
                        self.index_string_message -= 5
                        return int(self.string_message[self.index_string_message:self.index_string_message + 8],
                                   2) + 13, 12

    def handle_ordinal(self):
        string_cur_code = self.string_message[self.index_string_message - self.params[1][4]:self.index_string_message]
        self.history.append('{:0>8}'.format(string_cur_code))
        self.index_history += 1
        if self.flag_pre_code == 0:  # previous code is an ordinal
            self.list_string_collection.append(self.new_string_collection(self.index_history - 1, 2))
        elif self.flag_pre_code == 1:  # previous code is a codeword
            pre_codeword = int(
                self.string_message[self.index_string_message + 2:self.index_string_message + 2 + self.params[1][1]], 2)
            pre_string_collection = self.search_string_collection_by_codeword(pre_codeword)
            length_pre_string = pre_string_collection['string_length']
            self.list_string_collection.append(
                self.new_string_collection(self.index_history - 1, length_pre_string + 1))
        elif self.flag_pre_code == 2:  # previous code is a string-extension length
            ...
        elif self.flag_pre_code == 3:  # previous code is FLUSH control code
            if self.flag_first_two_code == 0:
                self.list_string_collection.append(self.new_string_collection(self.index_history - 1, 2))
            elif self.flag_first_two_code == 1:
                pre_codeword = int(self.string_message[
                                   self.index_string_message + 2 + self.params[1][1]:2 * self.params[1][
                                       1] + self.index_string_message + 2], 2)
                pre_string_collection = self.search_string_collection_by_codeword(pre_codeword)
                length_pre_string = pre_string_collection['string_length']
                self.list_string_collection.append(
                    self.new_string_collection(self.index_history - 1, length_pre_string + 1))
        elif self.flag_pre_code == -1:  # current code is the first code
            ...
        self.index_string_message -= self.params[1][4] + 1

    def handle_string_extension_length(self):
        tup_tmp = self.transfer_string_extension_length()
        string_seg_length = tup_tmp[0]
        bit_cost = tup_tmp[1]  # bit cost to indicate the string-extension length
        pre_codeword = int(self.string_message[
                           self.index_string_message + 2 + bit_cost:
                           self.index_string_message + 2 + bit_cost + self.params[1][1]], 2)
        pre_string_collection = self.search_string_collection_by_codeword(pre_codeword)
        last_char_pos_pre_string = pre_string_collection['last_char_pos']
        length_pre_string = pre_string_collection['string_length']
        while string_seg_length > 0:
            self.history.append(self.history[last_char_pos_pre_string + 1])
            last_char_pos_pre_string += 1
            self.index_history += 1
            string_seg_length -= 1
        self.list_string_collection.append(self.new_string_collection(
            self.index_history - 1, tup_tmp[0] + length_pre_string))
        self.index_string_message -= 1

    def handle_codeword(self, int_cur_codeword):
        if int_cur_codeword < self.params[1][0]:  # current codeword is less than C1
            string_collection_cur_codeword = self.search_string_collection_by_codeword(int_cur_codeword)
            length_cur_string = string_collection_cur_codeword['string_length']
            last_char_pos_pre_string = string_collection_cur_codeword['last_char_pos']
            first_char_pos_pre_string = 1 + last_char_pos_pre_string - length_cur_string
            for i in range(length_cur_string):
                self.history.append(self.history[first_char_pos_pre_string + i])
                self.index_history += 1
            if self.flag_pre_code == 0:  # previous code is an ordinal
                self.list_string_collection.append(
                    self.new_string_collection(self.index_history - length_cur_string, 2))
            elif self.flag_pre_code == 1:  # previous code is a codeword
                codeword_pre_string = int(self.string_message[self.index_string_message + self.params[1][1] + 2:
                                                              self.index_string_message + 2 + self.params[1][1] * 2], 2)
                string_collection_pre_codeword = self.search_string_collection_by_codeword(codeword_pre_string)
                length_pre_string = string_collection_pre_codeword['string_length']
                self.list_string_collection.append(
                    self.new_string_collection(self.index_history - length_cur_string, length_pre_string + 1))
            elif self.flag_pre_code == 2:  # previous code is a string-extension length
                ...
            elif self.flag_pre_code == 3:  # previous code is a FLUSH control code
                if self.flag_first_two_code == 0:
                    self.list_string_collection.append(
                        self.new_string_collection(self.index_history - length_cur_string, 2))
                elif self.flag_first_two_code == 1:
                    codeword_pre_string = int(self.string_message[self.index_string_message + 2 + 2 * self.params[1][
                        1]:self.index_string_message + 2 + 3 * self.params[1][1]], 2)
                    string_collection_pre_codeword = self.search_string_collection_by_codeword(codeword_pre_string)
                    length_pre_string = string_collection_pre_codeword['string_length']
                    self.list_string_collection.append(
                        self.new_string_collection(self.index_history - length_cur_string, length_pre_string + 1))
        elif int_cur_codeword == self.params[1][0]:  # current code is equal to C1
            if self.flag_pre_code == 0:  # previous code is an ordinal
                string_pre_ordinal = self.string_message[
                                     self.index_string_message + self.params[1][1] + 2:
                                     self.index_string_message + 2 + self.params[1][1] + self.params[1][4]]
                for i in range(2):
                    self.history.append('{:0>8}'.format(string_pre_ordinal))
                    self.index_history += 1
                self.list_string_collection.append(self.new_string_collection(self.index_history - 2, 2))
            elif self.flag_pre_code == 1:  # previous code is a codeword
                string_collection_pre_codeword = self.search_string_collection_by_codeword(
                    int(self.string_message[self.index_string_message + self.params[1][1] + 2:
                                            self.index_string_message + 2 + self.params[1][1] * 2], 2))
                length_pre_string = string_collection_pre_codeword['string_length']
                last_char_pos_pre_string = string_collection_pre_codeword['last_char_pos']
                first_char_pos_pre_string = 1 + last_char_pos_pre_string - length_pre_string
                for i in range(length_pre_string):
                    self.history.append(self.history[first_char_pos_pre_string + i])
                    self.index_history += 1
                self.history.append(self.history[first_char_pos_pre_string])
                self.index_history += 1
                self.list_string_collection.append(
                    self.new_string_collection(self.index_history - 1, length_pre_string + 1))

    def handle_control_code(self, int_control_code):
        if int_control_code == 0:  # ETM
            ...
        elif int_control_code == 1:  # FLUSH
            bit_decoded = self.len_string_message - self.index_string_message - 1
            if bit_decoded % 8 != 0:
                bit_padded = 8 - bit_decoded % 8
                self.index_string_message -= bit_padded
        elif int_control_code == 2:  # SETUP
            if self.string_message[self.index_string_message] == '0':  # next code is an ordinal
                self.params[1][4] += 1
            else:  # next code is a codeword
                self.params[1][1] += 1
                self.params[1][2] *= 2
        elif int_control_code == 3:  # REINIT
            ...
