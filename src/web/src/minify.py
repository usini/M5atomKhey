#from css_html_js_minify import html_minify, css_minify
#from jsmin import jsmin
import os

def convertToGZIP(filename, varname, new_filename):

  with open(filename, 'r') as index_file:
    index_raw = index_file.read()

  # Minize file
  ext = os.path.splitext(filename)[1]

  if ext == ".css":
    print(filename + " ... " + " CSS")
    #index_minified = css_minify(index_raw)
  elif ext == ".js":
    print(filename + " ... " + " JS")
    #index_minified = jsmin(index_raw)
  else:
    print(filename + " ... " + " HTML")
    #index_minified = html_minify(index_raw)
    #index_minified = css_minify(index_minified)
    #index_minified = jsmin(index_minified)
    index_raw = index_raw.replace('@charset"utf-8";','')


  # Remove gz file if exists
  try:
    os.unlink(filename + ".gz")
  except:
    pass

  # Compress file (need gzip)
  try:
    os.system('gzip ' + filename)
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

convertToGZIP("index.html", "APP_INDEX", "../web_index.h")
convertToGZIP("qrcode.min.js", "APP_SCRIPT", "../web_script.h")
print("... Done!")