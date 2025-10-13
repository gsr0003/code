import os
from datetime import datetime
import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def decrypt3DES(file_path,file_name,dst_path):
    key=file_name[5:21]
    timestamp=os.path.getctime(file_path)
    dt=datetime.fromtimestamp(timestamp)
    iv=dt.strftime('%d%H%M%S')
    des=pyDes.triple_des(key.encode(),pyDes.CBC,iv.encode(),pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[22:],'wb') as decry_out:
            decry_out.write(des.decrypt(decry_in.read()))

def decryptAES(file_path,file_name,dst_path):
    key=file_name[4:20]
    timestamp=os.path.getctime(file_path)
    dt=datetime.fromtimestamp(timestamp)
    ctime=dt.strftime('%d%H%M%S')
    iv=f'{key[4:12]}{ctime}'
    aes=AES.new(key.encode(),AES.MODE_CBC,iv.encode())
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[21:],'wb') as decry_out:
            decry_out.write(aes.decrypt(decry_in.read()))