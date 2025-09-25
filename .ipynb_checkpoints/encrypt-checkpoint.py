import os
import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def encrypt3DES(file_path,key=b'0123456789ABCDEF',iv=b'abcdefgh'):
    des=pyDes.triple_des(key,pyDes.CBC,iv,pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path,'rb') as encry_in:
        with open(f'3DES_{file_path}','wb') as encry_out:
            encry_out.write(des.encrypt(encry_in.read()))

def encryptAES(file_path,key=b'0123456789ABCDEF',iv=b'abcdefghijklmnop'):
    aes=AES.new(key,AES.MODE_CBC,iv)
    with open(file_path,'rb') as encry_in:
        with open(f'AES_{file_path}','wb') as encry_out:
            encry_out.write(aes.encrypt(pad(encry_in.read(),AES.block_size)))