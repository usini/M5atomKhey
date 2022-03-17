Import("env", "projenv")
import os
import shutil
def before_upload(source, target, env):
    print("--------- After Build -------------------")
    source = env.get("PROJECT_BUILD_DIR") + "\\" + env.get("PIOENV")
    destination = os.getcwd() + "\\docs\\bins" + "\\" + env.get("PIOENV")
    shutil.copyfile(source + "\\firmware.bin", destination + "\\firmware.bin")
    shutil.copyfile(source + "\\partitions.bin", destination + "\\partitions.bin")
    print(source)
    print(destination)
    print("---------------------------------------")

def after_upload(source, target, env):
    pass

env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)
