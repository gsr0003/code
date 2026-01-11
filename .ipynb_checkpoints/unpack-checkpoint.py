import os
import zipfile
import rarfile
import tarfile
#解压zip文件
def unpackZipFile(file):
    #for file in file_list:
    #file_name=file[:-4]
    zipf=zipfile.ZipFile(file)
    zipf.extractall()
    zipf.close()

'''def extractRarFile(file):
    #for file in file_list:
    #file_name=file[:-4]
    rar=rarfile.RarFile(file)
    rar.extractall()
    rar.close()'''

def unpackTarFile(file):
    with tarfile.open(file,'r:gz') as tar:
        tar.extractall()