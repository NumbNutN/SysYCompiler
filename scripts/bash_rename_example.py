import os

functional_dir = "./functional/"
hidden_functional_dir = "./hidden_functional/"


files = os.listdir(hidden_functional_dir)

import re

for file in files:
    order_lst = re.findall(r"\d+",file)
    if(order_lst != []):
        index = int(order_lst[0])
        newfile = re.sub(r"\d+",str(index+60),file,1)
        os.rename(hidden_functional_dir+file,hidden_functional_dir+newfile)
