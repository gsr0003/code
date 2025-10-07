import pyDes
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def decrypt3DES(file_path,file_name,dst_path,key=b'0123456789ABCDEF',iv=b'abcdefgh'):
    des=pyDes.triple_des(key,pyDes.CBC,iv,pad=None,padmode=pyDes.PAD_PKCS5)
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[5:],'wb') as decry_out:
            decry_out.write(des.decrypt(decry_in.read()))

def decryptAES(file_path,file_name,dst_path,key=b'0123456789ABCDEF',iv=b'abcdefghijklmnop'):
    aes=AES.new(key,AES.MODE_CBC,iv)
    with open(file_path+file_name,'rb') as decry_in:
        with open(dst_path+file_name[4:],'wb') as decry_out:
            decry_out.write(aes.decrypt(decry_in.read()))