import shutil
import os
import time
import oss2
import schedule

def backupLocal(path='backup'):
    '''if not os.path.exists(backup_path):
        os.makedirs(backup_path)
    date_str=time.strftime('%Y%m%d%H%M%S')
    backup_file=os.path.join(backup_path,f'backup_{date_str}.zip')
    shutil.make_archive(base_name=backup_file.replace('.zip',''),
                       format='zip',
                       root_dir=file_path)
    return backup_file'''
    files=[file for file in os.listdir('.') if os.path.isfile(file)]
    if not os.path.exists(path):
        os.makedirs(path)
    #os.chdir('../')
    os.chdir(path)
    date=time.strftime('%Y%m%d%H%M%S')
    os.makedirs(f'{date}')
    for file in files:
        #print(f'{path}/{date}/{file}')
        shutil.copy(f'../{file}',f'../{path}/{date}/{file}')
    os.chdir('../')