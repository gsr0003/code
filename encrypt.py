import os
import time
import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def encrypt3DES(file_path,file_name,key):
    iv=time.strftime('%d%H%M%S')
    des=pyDes.triple_des(key.encode(),pyDes.CBC,iv.encode(),pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path+file_name,'rb') as encry_in:
        with open(f'{file_path}3DES_{key}_{file_name}','wb') as encry_out:
            encry_out.write(des.encrypt(encry_in.read()))

def encryptAES(file_path,file_name,key):
    ctime=time.strftime('%d%H%M%S')
    iv=f'{key[4:12]}{ctime}'
    aes=AES.new(key.encode(),AES.MODE_CBC,iv.encode())
    with open(file_path+file_name,'rb') as encry_in:
        with open(f'{file_path}AES_{key}_{file_name}','wb') as encry_out:
            encry_out.write(aes.encrypt(pad(encry_in.read(),AES.block_size)))