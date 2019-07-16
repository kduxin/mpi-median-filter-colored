import os
from PIL import Image
import numpy as np
import struct



for file in os.listdir("./image/"):
    if not file.startswith('.'):
        img = Image.open("./image/" + file)
        img = np.array(img)
        
        h,w,c = img.shape
        img = [c for row in img for col in row for c in col]
        with open("./binimage/" + file + ".bin", "wb") as f:
            f.write(struct.pack("3i", h, w, c))
            f.write(struct.pack("%dB"%(h*w*c), *img))
        print("Finished saving image to `%s`"%("./binimage/" + file + ".bin"))
        