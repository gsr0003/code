import lzma

#lzma压缩，将文件以lzma算法压缩到添加了相应前缀名的新文件
def lzmaCompress(source_path,source_name):
    with open(source_path+source_name,'rb') as fp_in:
        with lzma.open(f'{source_path}LZMA_{source_name}','wb') as fp_out:
            fp_out.write(fp_in.read())

#lzma解压，将文件以lzma算法解压到去除了相应前缀名的新文件
def lzmaDecompress(source_path,source_name):
    with lzma.open(source_path+source_name,'rb') as fp_in:
        with open(source_path+source_name[5:],'wb') as fp_out:
            fp_out.write(fp_in.read())