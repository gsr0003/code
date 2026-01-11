import zipfile
import os
import tarfile
from rarfile import RarFile
from pathlib import Path
#文件压缩
def packZipFile(file_list,zip_name):
    with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for file in file_list:
            zipf.write(file)
            if Path(file).is_dir():
                for root, dirs, files in os.walk(file):
                    for Dir in dirs:
                        file_path=os.path.join(root,Dir)
                        zipf.write(file_path,file_path)
                    for File in files:
                        file_path = os.path.join(root, File)
                        zipf.write(file_path,file_path)
                
    
'''def compressRarFile(file_list,rar_name):
    with RarFile(rar_name,'w') as rar:
        for file in file_list:
            rar.write(file)
            if Path(file).is_dir():
                for root, dirs, files in os.walk(file):
                    for Dir in dirs:
                        file_path=os.path.join(root,Dir)
                        rar.write(file_path,file_path)
                    for File in files:
                        file_path = os.path.join(root, File)
                        rar.write(file_path,file_path)'''
    
def packTarFile(file_list,tgz_name):
    with tarfile.open(tgz_name,'w:gz') as tar:
        for file in file_list:
            tar.add(file,arcname=os.path.basename(file))#,arcname=file.split('/')[-1])
            '''if Path(file).is_dir():
                for root, dirs, files in os.walk(file):
                    for Dir in dirs:
                        file_path=os.path.join(root,Dir)
                        tar.add(file_path,file_path)
                    for File in files:
                        file_path = os.path.join(root, File)
                        tar.add(file_path,file_path)'''