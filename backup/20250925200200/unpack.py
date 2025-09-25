import os
import zipfile
import rarfile
import tarfile
#解压zip文件
def unpackZipFile(path,file):
    #for file in file_list:
    #file_name=file[:-4]
    zipf=zipfile.ZipFile(path+file)
    zipf.extractall(path)
    zipf.close()

'''def extractRarFile(file):
    #for file in file_list:
    #file_name=file[:-4]
    rar=rarfile.RarFile(file)
    rar.extractall()
    rar.close()'''

def unpackTarFile(path,file):
    with tarfile.open(path+file,'r:gz') as tar:
        tar.extractall(path)