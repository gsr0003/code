from tqdm import tqdm
from typing import Dict,List,Tuple

#创建节点类型
class Node:
    def __init__(self,value,weight,lchild,rchild):
        self.value=value
        self.weight=weight
        self.lchild=lchild
        self.rchild=rchild

#
def int_to_bytes(n:int)->bytes:
    return bytes([n])

#
def bytes_fre(bytes_str:bytes):
    fre_dic=[0 for _ in range(256)]
    for i in bytes_str:
        fre_dic[i]+=1
    return {int_to_bytes(x):fre_dic[x] for x in range(256) if fre_dic[x]>0}

#构建huffman树
def build(fre_dic:Dict[bytes,int])->Dict[bytes,str]:

    #
    def dlr(current:Node,huffman_code:str,huffman_dic:Dict[bytes,str]):
        if current is None:
            return
        if current.lchild is None and current.rchild is None:
            huffman_dic[current.value]=huffman_code
        else:
            dlr(current.lchild,huffman_code+'0',huffman_dic)
            dlr(current.rchild,huffman_code+'1',huffman_dic)

    if not fre_dic:
        return {}
    elif len(fre_dic)==1:
        return {value:'0' for value in fre_dic.keys()}
    node_lst=[Node(value,weight,None,None) for value,weight in fre_dic.items()]
    node_lst.sort(key=lambda item:item.weight,reverse=True)
    while len(node_lst)>1:
        node2=node_lst.pop()
        node1=node_lst.pop()
        node_add=Node(None,node1.weight+node2.weight,node1,node2)
        node_lst.append(node_add)
        index=len(node_lst)-1
        while index and node_lst[index-1].weight<=node_add.weight:
            node_lst[index]=node_lst[index-1]
            index=index-1
        node_lst[index]=node_add
    huffman_dic={key:'' for key in fre_dic.keys()}
    dlr(node_lst[0],'',huffman_dic)
    return huffman_dic

#
def toCanonical(huffman_dic:Dict[bytes,str])->Dict[bytes,str]:
    code_lst=[(value,len(code)) for value,code in huffman_dic.items()]
    code_lst.sort(key=lambda item:(item[1],item[0]),reverse=False)
    val_lst,len_lst=[],[]
    for val,length in code_lst:
        val_lst.append(val)
        len_lst.append(length)
    return rebuild(val_lst,len_lst)

#
def rebuild(val_lst:List[bytes],len_lst:List[int])->Dict[bytes,str]:
    huffman_dic={val:'' for val in val_lst}
    current_code=0
    for i in range(len(val_lst)):
        if i==0:
            current_code=0
        else:
            current_code=(current_code+1)<<(len_lst[i]-len_lst[i-1])
        huffman_dic[val_lst[i]]=bin(current_code)[2::].rjust(len_lst[i],'0')
    return huffman_dic

#
def encode(str_bytes:bytes,huffman_dic:Dict[bytes,str])->Tuple[bytes,int]:
    bin_buffer=''
    padding=0
    dic=[int_to_bytes(i) for i in range(256)]
    read_buffer=[dic[item] for item in str_bytes]
    write_buffer=bytearray([])
    for i in tqdm(read_buffer,unit='byte'):
        bin_buffer=bin_buffer+huffman_dic[i]
        while len(bin_buffer)>=8:
            write_buffer.append(int(bin_buffer[:8:],2))
            bin_buffer=bin_buffer[8::]

    if bin_buffer:
        padding=8-len(bin_buffer)
        bin_buffer=bin_buffer.ljust(8,'0')
        write_buffer.append(int(bin_buffer,2))
    return bytes(write_buffer),padding

#    
def decode(str_bytes:bytes,huffman_dic:Dict[bytes,str],padding:int):
    if not huffman_dic:
        return b''
    elif len(huffman_dic)==1:
        huffman_dic[b'OVO']='OVO'
    node_lst=[Node(value,weight,None,None) for value,weight in huffman_dic.items()]
    node_lst.sort(key=lambda i:(len(i.weight),i.weight),reverse=False)
    while len(node_lst)>1:
        node2=node_lst.pop()
        node1=node_lst.pop()
        node_add=Node(None,node1.weight[:-1:],node1,node2)
        node_lst.append(node_add)
        node_lst.sort(key=lambda i:(len(i.weight),i.weight),reverse=False)
    read_buffer,buffer_size=[],0
    dic=[list(map(int,bin(i)[2::].rjust(8,'0'))) for i in range(256)]
    for i in str_bytes:
        read_buffer.extend(dic[i])
        buffer_size=buffer_size+8
    read_buffer=read_buffer[0:buffer_size-padding:]
    buffer_size=buffer_size-padding
    write_buffer=bytearray([])
    current=node_lst[0]
    for pos in tqdm(range(0,buffer_size,8),unit='byte'):
        for i in read_buffer[pos:pos+8]:
            if i:
                current=current.rchild
            else:
                current=current.lchild
            if current.lchild is None and current.rchild is None:
                write_buffer.extend(current.value)
                current=node_lst[0]
    return bytes(write_buffer)

#huffman编码，将源内容按所构建的huffman树转换为huffman编码
def huffmanEncode(str_bytes:bytes,mode:int):
    fre_dic=bytes_fre(str_bytes)
    code_dic=build(fre_dic)
    code_dic=toCanonical(code_dic)
    max_len=0
    for code in code_dic.values():
        max_len=max(max_len,len(code))
    len_lst=[0 for _ in range(max_len+1)]
    for code in code_dic.values():
        len_lst[len(code)]+=1
    if len_lst[max_len]==256:
        len_lst[max_len]=0
    len_lst.pop(0)
    code_bytes=b''.join(code_dic.keys())
    len_bytes=b''.join(map(int_to_bytes,len_lst))
    temp_buffer,padding=encode(str_bytes,code_dic)
    code_data=int_to_bytes(max_len)+len_bytes+code_bytes
    write_buffer=int_to_bytes(padding)+code_data+temp_buffer
    return write_buffer

#huffman解码，将huffman编码按所构建的huffman树还原为原内容
def huffmanDecode(str_bytes:bytes,mode:int):
    #print(str_bytes)
    padding=str_bytes[0]
    max_len=str_bytes[1]
    #print(padding,max_len)
    length=list(str_bytes[2:2+max_len:])
    char_num=sum(length)
    if char_num==0 and max_len!=0:
        char_num=256
        length[max_len-1]=256
    char_lst,len_lst=[],[]
    for pos in range(2+max_len,2+max_len+char_num):
        char_lst.append(int_to_bytes(str_bytes[pos]))
    for i in range(max_len):
        len_lst.extend([i+1]*length[i])
    code_dic=rebuild(char_lst,len_lst)
    str_bytes=str_bytes[2+max_len+char_num::]
    write_buffer=decode(str_bytes,code_dic,padding)
    return write_buffer

#huffman压缩，将文件以huffman算法压缩到添加了相应前缀名的新文件
def huffmanCompress(source_path:str,source_name:str,mode:int=0):
    with open(source_path+source_name,'rb') as fp_in:
        with open(f'{source_path}huffman_{source_name}','wb') as fp_out:
            write_buffer=huffmanEncode(fp_in.read(),mode)
            fp_out.write(write_buffer)

#huffman解压，将文件以huffman算法解压到去除了相应前缀名的新文件
def huffmanDecompress(source_path:str,source_name:str,mode:int=0):
    with open(source_path+source_name,'rb') as fp_in:
        with open(source_path+source_name[8:],'wb') as fp_out:
            write_buffer=huffmanDecode(fp_in.read(),mode)
            fp_out.write(write_buffer)