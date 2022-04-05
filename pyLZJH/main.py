# coding = utf-8

import os

from lzjh import LZJHEncoder, LZJHDecoder


def format_lzjh_file(path_file):
    with open(path_file, mode='rb') as f_obj:
        content_file = f_obj.readline()
    list_content_file = [content_file[2 * i:2 + 2 * i].decode('utf-8') for i
                         in range(int(len(content_file) / 2))]

    name_and_expand = os.path.splitext(path_file)
    path_analysis_file = name_and_expand[0] + 'Analysis' + name_and_expand[1]
    with open(path_analysis_file, mode='w') as f_obj:
        for l_c_f in list_content_file:
            f_obj.write(l_c_f + ' ' + '{:0>8}'.format(bin(int(l_c_f, 16))[2:]) + '\n')

    path_analysis_file = name_and_expand[0] + 'AnalysisBit' + name_and_expand[1]
    with open(path_analysis_file, mode='w') as f_obj:
        for l_c_f in list_content_file:
            f_obj.write("'" + '{:0>8}'.format(bin(int(l_c_f, 16))[2:]) + "'" + ", ")


# noinspection DuplicatedCode
def write_encode_result_to_file(list_encode_result):
    string_encode_result = ''
    for encode_result in reversed(list_encode_result):
        if type(encode_result[1]) is not tuple:
            string_encode_result += encode_result[1]
        else:
            for e_r in reversed(encode_result[1]):
                string_encode_result += e_r
    # print(len(string_encode_result))
    # print(string_encode_result)
    string_encode_result_padded = string_encode_result
    if len(string_encode_result) % 8 != 0:
        string_encode_result_padded = '{0:0>{1}}'.format(string_encode_result, 8 * (len(string_encode_result) // 8 + 1))

    length_string_encode_result_padded = int(len(string_encode_result_padded) / 8)
    with open('messageCompressResultAnalysis.txt', mode='w') as f_obj:
        for iser in range(length_string_encode_result_padded):
            cur_bin = string_encode_result_padded[length_string_encode_result_padded * 8 - 8 - 8 * iser:
                                                  length_string_encode_result_padded * 8 - 8 * iser]
            if cur_bin != '':
                f_obj.write('{:0>2}'.format(hex(int(cur_bin, 2))[2:].upper()) + ' ' + cur_bin + '\n')

    with open('messageCompressed.txt', mode='w') as f_obj:
        for iser in range(length_string_encode_result_padded):
            cur_bin = string_encode_result_padded[length_string_encode_result_padded * 8 - 8 - 8 * iser:
                                                  length_string_encode_result_padded * 8 - 8 * iser]
            if cur_bin != '':
                f_obj.write('{:0>2}'.format(hex(int(cur_bin, 2))[2:].upper()))


# noinspection DuplicatedCode
def compress_message_file(list_param, file_path):
    with open(file_path, mode='r') as f_obj:
        string_buffer = f_obj.readline()
    list_message_data = ['{:0>8}'.format(bin(int(string_buffer[i * 2:i * 2 + 2], 16))[2:]) for i in
                         range(int(len(string_buffer) / 2))]
    # print(list_message_data)

    lzjh_encoder = LZJHEncoder(list_param, list_message_data)
    generator_encode_result = lzjh_encoder.encode()

    list_encode_result = []
    # print("Encode result:")
    for encode_result in generator_encode_result:
        list_encode_result.append(encode_result)
        # print(encode_result)

    for index_lcr in range(int(len(list_encode_result) / 2)):
        print(list_encode_result[2 * index_lcr + 1])
    write_encode_result_to_file(list_encode_result)

    print("Compressed successfully")


# noinspection DuplicatedCode
def write_decode_result_to_file(list_decode_result):
    # 写入文件
    with open('messageDecompressResult.txt', mode='w') as f_obj:
        for decode_result in list_decode_result:
            f_obj.write('{:0>2}'.format(hex(int(decode_result, 2))[2:].upper()))

    with open('messageDecompressResultAnalysis.txt', mode='w') as f_obj:
        for decode_result in list_decode_result:
            f_obj.write('{:0>2}'.format(hex(int(decode_result, 2))[2:].upper()) + ' ' + decode_result + '\n')


# noinspection DuplicatedCode
def decompress_message_compressed_file(list_param, file_path):
    with open(file_path, mode='r') as f_obj:
        string_message_compressed = f_obj.readline()
    list_bin_message_compressed = ['{:0>8}'.format(bin(int(string_message_compressed[i * 2:i * 2 + 2], 16))[2:]) for i
                                   in
                                   range(int(len(string_message_compressed) / 2))]
    string_message_compressed_formatted = ''
    for bin_message_compressed in reversed(list_bin_message_compressed):
        string_message_compressed_formatted += ''.join(bin_message_compressed)

    lzjh_decoder = LZJHDecoder(list_param, string_message_compressed_formatted)
    list_decode_result = lzjh_decoder.decode()
    # print('Decode result:')
    # for decode_result in list_decode_result:
    #     print(decode_result)

    # 写入文件
    write_decode_result_to_file(list_decode_result)

    print("Decompressed successfully")


if __name__ == '__main__':
    format_lzjh_file('message.txt')
    format_lzjh_file('messageCompressed.txt')

    '''
        params[0](length = 8) means the operating parameters for data compression function
        params[1](length = 5) means the operating variable for data compression function
    '''
    params = [[]] * 2
    params[0] = [10, 1024, 8, 256, 4, 0, 255, 3 * 1024]
    params[1] = [4, 6, 64, 0, 7]

    params1 = [[]] * 2
    params1[0] = [10, 1024, 8, 256, 4, 0, 255, 3 * 1024]
    params1[1] = [4, 6, 64, 0, 7]

    choice = '0'
    print("Input your choice\n1.Compress Mode\n2.Decompress Mode\n3.Quit")
    while choice != '3':
        choice = input("Your choice:")
        if choice == '1':
            compress_message_file(params, 'message.txt')
        elif choice == '2':
            decompress_message_compressed_file(params1, 'messageCompressed.txt')
        elif choice == '3':
            break
        else:
            print("Check your choice")
