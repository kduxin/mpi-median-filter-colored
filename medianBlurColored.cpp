
#include <cstdio>
#include <algorithm>
#include <memory>
#include <time.h>

typedef unsigned char uint8;

int partition(uint8* array, int low, int up);
void quickSort(uint8* array, int low, int up);


// quick sort : partition(), quickSort()
int partition(uint8* array, int low, int up)
{
	int pivot = array[up];
	int i = low-1;
	for (int j = low; j < up; j++)
	{
		if(array[j] <= pivot)
		{
			i++;
			std::swap(array[i], array[j]);
		}
	}
	std::swap(array[i+1], array[up]);
	return i+1;
}

void quickSort(uint8* array, int low, int up)
{
	if(low < up)
	{
		int mid = partition(array, low, up);
		//Watch out! The mid position is on the place, so we don't need to consider it again.
		//That's why below is mid-1, not mid! Otherwise it will occur overflow error!!!
		quickSort(array, low, mid-1);
		quickSort(array, mid+1, up);
	}
}


// main function
extern "C" float medianBlurColored(uint8* image, int height, int width, int channel, int window_size) {
    // median blur algorithm
    // Input: 
    //     image: 1-d array representing the image to be processed
    //            length = image.height * image.width * image.channel
    //            index_of_point = (y*width+x)*channel + c
    //              where c is the rgb channel index
    //     height: image's height
    //     width: image's width
    //     channel: image's channel count = 3
    //     window_size: parameter of MedianBlur algorithm (should be odd number)
    // Output:
    //     image: the image will be processed in place
    //     (return): the time of processing (in miliseconds)
    if (window_size % 2 == 0) {
        printf("Please give odd window_size like 3,5,7...\n");
        return 0;
    }
    clock_t t0 = clock();
    uint8* image_o = new uint8[height*width*channel];

    int margin = (window_size-1)/2;
    int c_offset = 0;
    int cache_length = 0;
    int i = 0;
    int patch_h_start(0), patch_h_end(0), patch_w_start(0), patch_w_end(0);
    int patch_height(0), patch_width(0);
    uint8* cache = new uint8[window_size*window_size];
    clock_t t1 = clock();
    for (int c = 0; c < channel; c++) {
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                patch_h_start = std::max(0, h-margin);
                patch_h_end   = std::min(height-1, h+margin);
                patch_w_start = std::max(0, w-margin);
                patch_w_end   = std::min(width-1, w+margin);
                i = 0;
                for (int p = patch_h_start; p < patch_h_end+1; p++) {
                    for (int q = patch_w_start; q < patch_w_end+1; q++) {
                        cache[i] = image[c+(p*width+q)*channel];
                        ++i;
                    }
                }
                cache_length = i;
                quickSort(cache, 0, cache_length-1);
                image_o[c+(h*width+w)*channel] = cache[cache_length/2];
            }
        }
    }
    clock_t t2 = clock();
    for (int i = 0; i < height*width*channel; i++) {
        image[i] = image_o[i];
    }
    delete[] image_o;
    clock_t t3 = clock();
    
    printf("memory preprocess: %5.2f\n", (t1-t0)*1.0/CLOCKS_PER_SEC*1000);
    printf("core process: %5.2f\n", (t2-t1)*1.0/CLOCKS_PER_SEC*1000);
    printf("memory postprocess: %5.2f\n", (t3-t2)*1.0/CLOCKS_PER_SEC*1000);
    return (float)(t3-t0)*1.0/CLOCKS_PER_SEC*1000;
}

// compile: g++ medianBlurColored.cpp -fPIC -shared -o lib_cpu.so
