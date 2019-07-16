import ctypes
from PIL import Image
import numpy as np
import cv2


def SaltNoise(img, prob=0.01):
    shape = img.shape
    noise = np.random.rand(*shape)
    img_sn = img.copy()
    img_sn[noise <  prob] = 1
    return img_sn

for i in range(1,14):
    filename = "./image_ori/%d.jpg"%i
    sfilename="./image/%d.jpg"%i

    img = np.array(Image.open(filename), dtype=np.uint8)
    img = SaltNoise(img)
    img = Image.fromarray(img)
    img.save(sfilename)