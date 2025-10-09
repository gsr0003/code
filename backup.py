import shutil
import os
import time
#import schedule
import git

def backupLocal(path='../backup'):
    files=[file for file in os.listdir('.') if os.path.isfile(file)]
    if not os.path.exists(path):
        os.makedirs(path)
    date=time.strftime('%Y%m%d%H%M%S')
    os.makedirs(f'{path}/{date}')
    for file in files:
        shutil.copy(file,f'{path}/{date}/{file}')

'''def backupGitHub(path='C:\\Users\\gsr00\\软工\\code'):
    try:
        repo=git.Repo(path)
    except:
        repo.git.Repo.init(path)
    repo.git.add('-A')
    date=time.strftime('%Y.%m.%d %H:%M:%S')
    commit_message=f'Automatically update at {date}'
    repo.index.commit(commit_message)
    origin=repo.remote(name='origin')
    origin.push()'''
def backupGitHub(path='C:\\Users\\gsr00\\软工\\code'):
    try:
        repo=git.Repo(path)
    except:
        repo.git.Repo.init(path)
    #new_url='https://github.com/gsr0003/code.git'
    origin=repo.remote('origin')
    #origin.set_url(new_url)
    repo.git.add('-A')
    date=time.strftime('%Y.%m.%d %H:%M:%S')
    commit_message=f'Automatically update at {date}'
    repo.index.commit(commit_message)
    print("开始推送...")
    origin.push()
    print('推送成功')
