import os
from datetime import datetime
import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

#3DES解密，将以加密文件以3DES算法解密到去除相关前缀的新文件中
def decrypt3DES(file_path,file_name,dst_path):
    #从文件名中提取出密钥
    key=file_name[5:21]

    #获取文件创建时间，以求出盐值
    timestamp=os.path.getctime(file_path+file_name)
    dt=datetime.fromtimestamp(timestamp)
    iv=dt.strftime('%d%H%M%S')
    
    des=pyDes.triple_des(key.encode(),pyDes.CBC,iv.encode(),pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[22:],'wb') as decry_out:
            decry_out.write(des.decrypt(decry_in.read()))

#AES解密，将以加密文件以AES算法解密到去除相关前缀的新文件中
def decryptAES(file_path,file_name,dst_path):
    #从文件名中提取出密钥
    key=file_name[4:20]
    
    #获取文件创建时间，以求出盐值
    timestamp=os.path.getctime(file_path+file_name)
    dt=datetime.fromtimestamp(timestamp)
    ctime=dt.strftime('%d%H%M%S')
    iv=f'{key[4:12]}{ctime}'
    
    aes=AES.new(key.encode(),AES.MODE_CBC,iv.encode())
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[21:],'wb') as decry_out:
            decry_out.write(aes.decrypt(decry_in.read()))