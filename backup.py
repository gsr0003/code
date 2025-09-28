import shutil
import os
import time
import schedule
import git

def backupLocal(path='../backup'):
    files=[file for file in os.listdir('.') if os.path.isfile(file)]
    if not os.path.exists(path):
        os.makedirs(path)
    date=time.strftime('%Y%m%d%H%M%S')
    os.makedirs(f'{path}/{date}')
    for file in files:
        shutil.copy(file,f'{path}/{date}/{file}')

def backupGitHub(path='.'):
    try:
        repo=git.Repo(path)
    except:
        repo.git.Repo.init(path)
    repo.git.add('-A')
    date=time.strftime('%Y%m%d%H%M%S')
    commit_message=f'Automated commit message at{date}'
    repo.index.commit(commit_message)
    origin=repo.remote(name='origin')
    origin.push()