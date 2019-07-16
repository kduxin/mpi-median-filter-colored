
import ctypes
from PIL import Image
import numpy as np
import cv2
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# lib_cpu = ctypes.cdll.LoadLibrary("./lib_cpu.so")
lib_mpi = ctypes.cdll.LoadLibrary("./lib_mpi.so")

img = np.array(Image.open("./image/aimer.jpg"), dtype=np.uint8)
# img = img.transpose([2,0,1])
shape = img.shape


def SaltNoise(img, prob=0.01):
    shape = img.shape
    noise = np.random.rand(*shape)
    img_sn = img.copy()
    img_sn[noise <  prob] = 1
    return img_sn

def MedianBlurCpu(img, window_size):
    c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
    shape = img.shape
    img_p = img_sn.reshape([-1]).copy().ctypes.data_as(c_uint8_p)
    lib_cpu.medianBlurColored(img_p, shape[0], shape[1], shape[2], window_size)
    print("44444")
    img_f = np.array(img_p[:shape[0]*shape[1]*shape[2]], dtype=np.uint8).reshape(*shape)
    print("55555")
    return img_f

def MedianBlurMPI(img, window_size):
    c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
    shape = img.shape
    img_p = img_sn.reshape([-1]).copy().ctypes.data_as(c_uint8_p)
    lib_mpi.medianBlurColored_MPI(img_p, shape[0], shape[1], shape[2], window_size)
    print("44444")
    img_f = np.array(img_p[:shape[0]*shape[1]*shape[2]], dtype=np.uint8).reshape(*shape)
    print("55555")
    return img_f

def MedianBlurCuda(img, window_size):
    c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
    shape = img.shape
    img_p = img_sn.reshape([-1]).copy().ctypes.data_as(c_uint8_p)
    lib.medianBlurColoredCuda(img_p, shape[0], shape[1], shape[2], window_size)
    img_f = np.array(img_p[:shape[0]*shape[1]*shape[2]], dtype=np.uint8).reshape(shape)
    return img_f


img_sn = SaltNoise(img, prob=0.01)
# img_medblur = cv2.medianBlur(img_sn, 5)
# img_bilblur = cv2.bilateralFilter(img_sn, 5, 75, 75)
# img_nlmblur = cv2.fastNlMeansDenoisingColored(img_sn, h=9, hColor=10)



print("Start medblurring with mpi.")
# img_medblur_cpu = MedianBlurCpu(img, 5)
img_medblur_mpi = MedianBlurMPI(img_sn, 5)
# img_medblur_cuda = MedianBlurCuda(img, 5)


# """ draw figure """
# fig = plt.figure(figsize=(15,10))
# ax11 = fig.add_subplot('221')
# ax12 = fig.add_subplot('222')
# ax21 = fig.add_subplot('223')
# ax22 = fig.add_subplot('224')
# ax11.imshow(img, aspect='auto')
# ax12.imshow(img_sn, aspect='auto')
# ax21.imshow(img_medblur_cpu, aspect='auto')
# ax22.imshow(img_medblur_mpi, aspect='auto')
# ax11.set_title('Raw Image')
# ax12.set_title('Salt Noise with prob. 0.1')
# ax21.set_title('Denoised Image with Median Blurring (single cpu)')
# ax22.set_title('Denoised Image with Median Blurring (mpi)')
# for ax in [ax11, ax12, ax21, ax22]:
#     ax.axis("off")
# fig.subplots_adjust(hspace=0.2, wspace=0.2)
# fig.save_fig("./result.jpg")




# plt.imshow(np.array(img, dtype=np.uint8))
# plt.show()
# plt.imshow(np.array(img_medblur_cpu, dtype=np.uint8))
# plt.show()

# lib.printList(img_p, 6)


# img_f = np.frombuffer(img_p, dtype=np.float32, count=3)

# img_f = np.fromiter(img_p, dtype=np.float32, count=6)
# img_f
# plt.imshow(img_f.reshape([400,-1]))
# plt.show()
# plt.imshow(img)


