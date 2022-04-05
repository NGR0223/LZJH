# encoding = utf-8

from lzjh import LZJHEncoder, LZJHDecoder


def read_file_binary(file_path):
    with open(file_path, "rb") as f_obj:
        data = f_obj.read()
    return data


def write_file_binary(file_path, data):
    with open(file_path, mode="wb") as f_obj:
        f_obj.write(data)


if __name__ == '__main__':
    with open('message.txt', mode='rb') as f:
        stringBuffer = f.read()
    listData = ['{:0>8}'.format(bin(int(stringBuffer[i * 2:i * 2 + 2].decode('utf-8'), 16))[2:]) for i in
                range(int(len(stringBuffer) / 2))]

    '''
        params[0](length = 8) means the operating parameters for data compression function
        params[1](length = 5) means the operating variable for data compression function
    '''
    params = [[]] * 2
    params[0] = [10, 1024, 8, 256, 4, 0, 255, 3 * 1024]
    params[1] = [4, 7, 64, 0, 8]
    lzjhEncoder = LZJHEncoder(params, listData)
    generatorEncodeResult = lzjhEncoder.encode()

    listEncodeResult = []
    # print("Encode result:")
    for encodeResult in generatorEncodeResult:
        listEncodeResult.append(encodeResult)
        # print(encodeResult[0])

    params1 = [[]] * 2
    params1[0] = [10, 1024, 8, 256, 4, 0, 255, 3 * 1024]
    params1[1] = [4, 7, 64, 0, 8]
    lzjhDecoder = LZJHDecoder(params1, listEncodeResult)

    print('Decode result:')
    listDecodeResult = lzjhDecoder.decode()
    if listDecodeResult == listData:
        print("True")
        for decodeResult in listDecodeResult:
            print(decodeResult)
    else:
        print("False")
        print(listData)
        print(listDecodeResult)
        print(len(listData), len(listDecodeResult))
        for i in range(len(listData)):
            if listData[i] != listDecodeResult[i]:
                print(i)
                break
