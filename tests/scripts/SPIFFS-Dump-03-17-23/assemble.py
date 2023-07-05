import base64, math
b32_string = "GAZS6MJYF4ZDGLJQGE5DEMZ2GU2SYLJRGE3S4OJZGY4DMLBWGAXDQLBTGMXDQNRQGIWC2MJRG4XDSOJWHA3A"
pad_length = math.ceil(len(b32_string) / 8) * 8 - len(b32_string)
bytes_data = base64.b32decode(b32_string.encode('ascii') + b'=' * pad_length)


print(bytes_data)

