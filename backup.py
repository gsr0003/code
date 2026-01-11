import shutil
import os
import time
#import schedule
import git
import subprocess

#本地自动备份，每次启动GUI界面时自动备份代码到本地专用文件夹
def backupLocal(path='../backup'):
    files=[file for file in os.listdir('.') if os.path.isfile(file)]
    if not os.path.exists(path):
        os.makedirs(path)
    date=time.strftime('%Y%m%d%H%M%S')
    os.makedirs(f'{path}/{date}')
    for file in files:
        shutil.copy(file,f'{path}/{date}/{file}')

#网络自动备份，每次启动GUI界面时自动备份代码到GitHub
def backupGitHub(path='C:/Users/gsr00/软工/code'):
    date=time.strftime('%Y.%m.%d %H:%M:%S')
    commit_message=f'Update at {date}'
    os.chdir(path)
    status=subprocess.run(['git','diff-index','--quiet','HEAD','--'])
    if status.returncode==0:
        print('No changes to commit')
    else:
        subprocess.run(['git','add','.'])
        subprocess.run(['git','commit','-m',commit_message])
        subprocess.run(['git','push','origin','master'])

