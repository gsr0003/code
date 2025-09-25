import lzma
def lzmaCompress(source_path,source_name):
    with open(source_path+source_name,'rb') as fp_in:
        with lzma.open(f'{source_path}LZMA_{source_name}','wb') as fp_out:
            fp_out.write(fp_in.read())

def lzmaDecompress(source_path,source_name):
    with lzma.open(source_path+source_name,'rb') as fp_in:
        with open(source_path+source_name[5:],'wb') as fp_out:
            fp_out.write(fp_in.read())