import os
import subprocess

def convertToGZIP(filename, varname, new_filename):
    with open(filename, 'r', encoding="utf8") as index_file:
        index_raw = index_file.read()

    # Minize file
    ext = os.path.splitext(filename)[1]
    if ext == ".css":
        print(filename + " ... " + " CSS")
    elif ext == ".js":
        print(filename + " ... " + " JS")
    else:
        print(filename + " ... " + " HTML")
        index_raw = index_raw.replace('@charset"utf-8";','')

    # Remove gz file if exists
    try:
        os.unlink(filename + ".gz")
    except:
        pass

    # Compress file (need gzip)
    try:
        command = 'gzip --keep ' + filename
        print(command)
        os.system(command)
    except:
        pass

    # Generate hex_str
    hex_str = "const uint8_t " + varname + "[] PROGMEM={"
    with open(filename + ".gz", 'rb') as f:
        block = f.read()
        for ch in block:
                hex_str += hex(ch) + ", "

    try:
        os.unlink(filename + ".gz")
    except:
        pass

    index_final = hex_str + "};"
    with open(new_filename,'w') as index_final_file:
        index_final_file.write(index_final)


def before_build():
    print("---------------------  Prepare Web Pages -------------------")
    convertToGZIP("src/web/src/index.html", "APP_INDEX", "src/web/web_index.h")
    convertToGZIP("src/web/src/qrcode.min.js", "APP_QRCODE", "src/web/web_qrcode.h")
    convertToGZIP("src/web/src/matrix.js", "APP_MATRIX", "src/web/web_matrix.h")
    convertToGZIP("src/web/src/style.css", "APP_STYLE", "src/web/web_style.h")
    print("... Done!")
    print("---------------------  Prepare Web Pages -------------------")

before_build()

