import os
import ctypes
from PIL import Image
import numpy as np
import pandas as pd
import cv2
import matplotlib.pyplot as plt


def SaltNoise(img, prob=0.01):
    # add salt noise to img
    shape = img.shape
    noise = np.random.rand(*shape)
    img_sn = img.copy()
    img_sn[noise <  prob] = 1
    return img_sn

def MedianBlurCpu(img, window_size):
    c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
    shape = img.shape
    img_p = img.reshape([-1]).copy().ctypes.data_as(c_uint8_p)
    lib_cpu.medianBlurColored.restype = ctypes.c_float
    milisec = lib_cpu.medianBlurColored(img_p, shape[0], shape[1], shape[2], window_size)
    img_f = np.array(img_p[:shape[0]*shape[1]*shape[2]], dtype=np.uint8).reshape(*shape)
    return img_f, milisec

# load dynamic linked library
lib_cpu = ctypes.cdll.LoadLibrary("./lib_cpu.so")
# load image and add salt noise to each image
imgs = []
for file in os.listdir('./image/'):
    if not file.startswith('.'):
        img = np.array(Image.open("./image/{}".format(file)), dtype=np.uint8)
        imgs.append((file, SaltNoise(img, prob=0.01)))

# run the median blur algorithm once in advance
MedianBlurCpu(imgs[0][1], 3)

# run the median blur algorithm on all images and record the process time 
process_time = []
rerun = 5
for i,(file,img) in enumerate(imgs):
    for window_size in [3]:
        print(i, window_size)
        t = []
        for r in range(rerun):
            _, milisec = MedianBlurCpu(img, window_size)
            t.append(milisec)
        process_time.append((file, img.shape[0], img.shape[1], window_size, np.mean(t), np.std(t)))
records_cpu = pd.DataFrame(process_time, columns=['file', 'height', 'width', 'window_size', 
                                              'time_mean (msec)', 'time_std'])
records_cpu.to_csv('./records_cpu.csv')