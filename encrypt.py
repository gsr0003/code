import os
import time
import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

#3DES加密，将文件以3DES算法加密到添加了加密方式及密钥的新文件中
def encrypt3DES(file_path,file_name,key):
    #获取当前时间作为盐值
    iv=time.strftime('%d%H%M%S')
    
    des=pyDes.triple_des(key.encode(),pyDes.CBC,iv.encode(),pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path+file_name,'rb') as encry_in:
        with open(f'{file_path}3DES_{key}_{file_name}','wb') as encry_out:
            encry_out.write(des.encrypt(encry_in.read()))

#AES加密，将文件以AES算法加密到添加了加密方式及密钥的新文件中
def encryptAES(file_path,file_name,key):
    #获取当前时间，与密钥中间8位合并作为盐值
    ctime=time.strftime('%d%H%M%S')
    iv=f'{key[4:12]}{ctime}'
    
    aes=AES.new(key.encode(),AES.MODE_CBC,iv.encode())
    with open(file_path+file_name,'rb') as encry_in:
        with open(f'{file_path}AES_{key}_{file_name}','wb') as encry_out:
            encry_out.write(aes.encrypt(pad(encry_in.read(),AES.block_size)))