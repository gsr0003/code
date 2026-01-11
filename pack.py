import zipfile
import os
import tarfile
from rarfile import RarFile
from pathlib import Path

#zip打包，将所选文件打包成一个zip文件
def packZipFile(file_path,file_list,dst_path,zip_name):
    with zipfile.ZipFile(dst_path+zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for file in file_list:
            File=f'{file_path}{file}'
            zipf.write(File,file)
            if Path(File).is_dir():
                for root, dirs, files in os.walk(File):
                    Root=root[len(file_path):]
                    for Dir in dirs:
                        zipf.write(os.path.join(root,Dir),os.path.join(Root,Dir))
                    for FIle in files:
                        zipf.write(os.path.join(root,FIle),os.path.join(Root,FIle))
                
    
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

#tar打包，将所选文件打包成一个tgz文件
def packTarFile(file_path,file_list,dst_path,tgz_name):
    with tarfile.open(dst_path+tgz_name,'w:gz') as tar:
        for file in file_list:
            tar.add(file_path+file,arcname=os.path.basename(file))#,arcname=file.split('/')[-1])