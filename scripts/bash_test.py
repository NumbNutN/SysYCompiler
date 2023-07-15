import os
import subprocess

test_dir = "../test_cases"
output_dir = "../output"
exec_path = "../compiler"

files = os.listdir(test_dir)
for fileName in files:
    input_file = os.path.join(test_dir,fileName)
    output_file = os.path.join(output_dir,fileName.replace(".c",".s"))
    subprocess.run([exec_path,'-S','-o',output_file,input_file])
