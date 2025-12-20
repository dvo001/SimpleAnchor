Import("env")
from subprocess import call

def after_upload(source, target, env):
    print("Uploading SPIFFS...")
    call(["pio", "run", "--target", "uploadfs"])

env.AddPostAction("upload", after_upload)
