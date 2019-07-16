import os
from PIL import Image
import numpy as np
import struct


imgs = []
for file in os.listdir("./binimage/"):
    if (not file.startswith('.')) and file.endswith('.bin'):
        with open("./binimage/" + file, "rb") as f:
            data = f.read()
        h, w, c = struct.unpack("3i", data[:12])
        print("Image size:", h, w, c)
        image = struct.unpack("%dB"%(h*w*c), data[12:])
        image = np.array(image, dtype=np.uint8).reshape(h, w, c)
        print("Finished loading image from `%s`"%("./binimage/" + file))
        Image.fromarray(image).save("./binimage/" + file + ".jpg")
        print("Finished saving image to %s"%("./binimage/" + file + ".jpg"))

