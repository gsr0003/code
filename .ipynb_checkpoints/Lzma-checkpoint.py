import lzma
def lzmaCompress(source_path):
    with open(source_path,'rb') as fp_in:
        with lzma.open(f'{source_path}.xz','wb') as fp_out:
            fp_out.write(fp_in.read())

def lzmaDecompress(source_path):
    with lzma.open(source_path,'rb') as fp_in:
        with open(source_path[:-3],'wb') as fp_out:
            #print(fp_in.read())
            fp_out.write(fp_in.read())