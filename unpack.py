import os
import zipfile
import rarfile
import tarfile

#zip解包，将zip文件中包含的源文件解包出来
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

#tar解包，将tgz文件中包含的源文件解包出来
def unpackTarFile(path,file):
    with tarfile.open(path+file,'r:gz') as tar:
        tar.extractall(path)