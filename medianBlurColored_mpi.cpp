// compile with "mpic++ -fopenmp -O3 medianBlurColored_mpi.cpp cv_imgproc.cpp -o mpi.o -lopencv_core -lopencv_highgui -lopencv_imgproc"


#include <cstdio>
#include <cstring>
#include <algorithm>
#include <math.h>
#include <memory>
#include <time.h>
#include <mpi.h>
#include <string>
#include <omp.h>

#include "cv_imgproc.h"

#define TAG 0
#define min(a,b) (a<b?a:b)

#ifndef UINT8
typedef unsigned char uint8;
#endif

int partition(uint8* array, int low, int up);
void quickSort(uint8* array, int low, int up);



struct Job {
    // The job includes the information to specify a block area
    // of the image, composed of a center area and the padding area.
    // The center area will be [ystart:ystart+h, xstart:xstart+w, cstart:cstart+c]
    // The whole area will be [ystart-padh:ystart+h+padh, xstart-padw:xstart+w+padw, cstart:cstart+c]

    int h; // height of area without pad
    int w; // width of area without pad
    int c; // channel
    int ystart;
    int xstart;
    int cstart;
    int padh; // upper and bottom padding
    int padw; // left and right padding
};



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
int medianBlurColoredJob(uint8* image, const Job* job) {
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

    // printf("medianBlurColoredJob entered.\n");
    int height, width, channel;
    int padh, padw;
    padh = job->padh; padw = job->padw;
    height = job->h+2*padh; width = job->w+2*padw; channel = job->c;

    int i(0);
    int cachelen = (2*padh+1)*(2*padw+1);
    uint8* cache;
    uint8* image_o = new uint8[height*width*channel];
    // printf("Ready for median blurring.\n");
    printf("height=%d, width=%d, channel=%d\n", height, width, channel);
    // printf("padh=%d, padw=%d\n", padh, padw);
    int h,w,c,p,q;

                // cache = new uint8[cachelen];
    #pragma  omp parallel for shared(image,image_o,channel,padh,height,padw,width,cachelen) \
        private(i,c,h,w,p,q,cache) default(none)
    for (c = 0; c < channel; c++) {
        for (h = padh; h < height-padh; h++) {
            for (w = padw; w < width-padw; w++) {
                i = 0;
                cache = (uint8*)calloc(cachelen, sizeof(uint8));
                for (p = h-padh; p < h+padh+1; p++) {
                    for (q = w-padw; q < w+padw+1; q++) {
                        cache[i] = image[c+(p*width+q)*channel];
                        ++i;
                    }
                }
                // printf("pixel: %d\n", cache[cachelen/2]);
                quickSort(cache, 0, cachelen-1);
                // printf("pixel: %d\n", cache[cachelen/2]);
                image_o[c+(h*width+w)*channel] = cache[cachelen/2];
                free(cache);
            }
        }
    }

                // delete[] cache;

    // printf("Finished median filt job.\n");
    // memcpy(image, image_o, sizeof(uint8)*height*width*channel);
    for (int i = 0; i < height*width*channel; i++) {
        image[i] = image_o[i];
    }
    delete[] image_o;
    // delete[] cache;
    
    // printf("memory preprocess: %5.2f\n", (t1-t0)*1.0/CLOCKS_PER_SEC*1000);
    // printf("core process: %5.2f\n", (t2-t1)*1.0/CLOCKS_PER_SEC*1000);
    // printf("memory postprocess: %5.2f\n", (t3-t2)*1.0/CLOCKS_PER_SEC*1000);
    // return (float)(t3-t0)*1.0/CLOCKS_PER_SEC*1000;
    return 0;
}



void jobSchedule1dim(int height, int width, int channel, int margin, int size, Job* jobs) {
    // TODO
    int nslave = size-1;
    if (height > width) {
        // divide image by rows
        int rowsperjob = (height+nslave-1) / nslave;
        for (int i=1; i<size; i++) {
            jobs[i].ystart = (i-1)*rowsperjob;
            jobs[i].xstart = 0;
            jobs[i].cstart = 0;
            jobs[i].c = channel;
            jobs[i].padh = margin;
            jobs[i].padw = margin;
            jobs[i].h = min(height, jobs[i].ystart+rowsperjob) - jobs[i].ystart;
            jobs[i].w = width;
        }
    } else {
        // divide image by cols
        int colsperjob = (width+nslave-1) / nslave;
        for (int j=1; j<size; j++) {
            jobs[j].xstart = (j-1)*colsperjob;
            jobs[j].ystart = 0;
            jobs[j].cstart = 0;
            jobs[j].c = channel;
            jobs[j].padh = margin;
            jobs[j].padw = margin;
            jobs[j].h = height;
            jobs[j].w = min(width, jobs[j].xstart+colsperjob) - jobs[j].xstart;
        }
    }

    for (int i=1; i<size; i++) {
        printf("Job schedule: h=%d,w=%d,c=%d,ystart=%d,xstart=%d,cstart=%d,padh=%d,padw=%d\n",
            jobs[i].h, jobs[i].w, jobs[i].c, jobs[i].ystart, jobs[i].xstart, jobs[i].cstart, jobs[i].padh, jobs[i].padw);
    }

}


void jobSchedule2dim(int height, int width, int channel, int margin, int size, Job* jobs) {
    int nslave = size-1;
    float whratio= (float)width / (float)height;
    int nblockh = (int)sqrtf((float)nslave/whratio);
    int nblockw = nslave / nblockh;
    int rowsperjob = (height+nblockh-1) / nblockh;
    int colsperjob = (width+nblockw-1) / nblockw;
    for (int h=0; h<nblockh; h++) {
        for (int w=0; w<nblockw; w++) {
            int procid = 1+h*nblockw+w;
            jobs[procid].ystart = h*rowsperjob;
            jobs[procid].xstart = w*colsperjob;
            jobs[procid].cstart = 0;
            jobs[procid].padh = margin;
            jobs[procid].padw = margin;
            jobs[procid].h = min(height, jobs[procid].ystart+colsperjob) - jobs[procid].ystart;
            jobs[procid].w = min(width, jobs[procid].xstart+rowsperjob) - jobs[procid].xstart;
            jobs[procid].c = channel;
        }
    }
    for (int i=1+nblockh*nblockw; i<size; i++) {
            jobs[i].ystart = 0;
            jobs[i].xstart = 0;
            jobs[i].cstart = 0;
            jobs[i].padh = 0;
            jobs[i].padw = 0;
            jobs[i].h = 0;
            jobs[i].w = 0;
    }
    for (int i=1; i<size; i++) {
        printf("Job schedule: h=%d,w=%d,c=%d,ystart=%d,xstart=%d,cstart=%d,padh=%d,padw=%d\n",
            jobs[i].h, jobs[i].w, jobs[i].c, jobs[i].ystart, jobs[i].xstart, jobs[i].cstart, jobs[i].padh, jobs[i].padw);
    }
}

void createBuff(uint8* image, int height, int width, int channel, Job* job, uint8* buff) {
    // TODO
    int h, w, c, ystart, xstart, cstart, padh, padw;
    int blockw, blockh;
    h=job->h; w=job->w; c=job->c;
    ystart=job->ystart; xstart=job->xstart; cstart=job->cstart;
    padh=job->padh; padw=job->padw;
    blockw = w+2*padw; blockh = h+2*padh;
    // printf("Creating buff: height=%d, width=%d, channel=%d, h=%d, w=%d, c=%d, ystart=%d, xstart=%d, cstart=%d, padh=%d, padw=%d, blockw=%d, blockh=%d\n", 
    //     height, width, channel, h,w,c,ystart,xstart,cstart,padh,padw,blockw,blockh);

    // int n0(0), nn0(0);
    for (int y=ystart-padh; y<ystart+h+padh; y++) {
        for (int x=xstart-padw; x<xstart+w+padw; x++) {
            for (int z=cstart; z<cstart+c; z++) {
                if (y<0 || y>=height || x<0 || x>=width) {
                    // n0++;
                    buff[c*(blockw*(y-ystart+padh)+x-xstart+padw)
                                +z-cstart] = 0;
                } else {
                    // nn0++;
                    buff[c*(blockw*(y-ystart+padh)+x-xstart+padw)
                                +z-cstart] = image[channel*(width*y+x)+z];
                }
            }
        }
    }
    // printf("n0=%d, nn0=%d\n", n0, nn0);
}

void registerBuff(uint8* image, int height, int width, int channel, Job* job, uint8* buff) {
    // TODO
    int h, w, c, ystart, xstart, cstart, padh, padw;
    int blockw, blockh;
    h=job->h; w=job->w; c=job->c;
    ystart=job->ystart; xstart=job->xstart; cstart=job->cstart;
    padh=job->padh; padw=job->padw;
    blockw = w+2*padw; blockh = h+2*padh;

    for (int y=ystart; y<ystart+h; y++) {
        for (int x=xstart; x<xstart+w; x++) {
            for (int z=cstart; z<cstart+c; z++) {
                image[channel*(y*width+x)+z] = 
                    buff[c*(blockw*(y-ystart+padh)+x-xstart+padw)
                        +z-cstart];
            }
        }
    }
}

// extern "C" float medianBlurColored_MPI(uint8* image, int height, int width, int channel, int window_size) {

// }



// compile: mpic++ medianBlurColored_mpi.cpp -fPIC -shared -o lib_mpi.so

// void getImgSize(std::string filename, int& h, int& w, int& c) {
//     FILE* fp = fopen(filename.c_str(), "rb");
//     fread(&h, 1, sizeof(int), fp);
//     fread(&w, 1, sizeof(int), fp);
//     fread(&c, 1, sizeof(int), fp);
//     fclose(fp);
//     printf("Image size: h=%d, w=%d, c=%d\n", h, w, c);
// }

// void readImg(std::string filename, int h, int w, int c, uint8* image) {
//     FILE* fp = fopen(filename.c_str(), "rb");
//     // image = new uint8[h*w*c];
//     // image = (uint8*)malloc(sizeof(uint8)*h*w*c);
//     fseek(fp, sizeof(int)*3, SEEK_SET);
//     fread(image, h*w*c, sizeof(uint8), fp);
//     fclose(fp);
//     printf("Image loaded.\n");
// }

// void saveImg(std::string filename, int h, int w, int c, uint8* image) {
//     FILE *fp = fopen(filename.c_str(), "wb");
//     fwrite(&h, 1, sizeof(int), fp);
//     fwrite(&w, 1, sizeof(int), fp);
//     fwrite(&c, 1, sizeof(int), fp);
//     fwrite(image, h*w*c, sizeof(uint8), fp);
//     printf("Image saved to %s.\n", filename.c_str());
// }


int main(int argc, char* args[]) {

    std::string fullname, name, suffix, nfullname;
    int window_size = atoi(args[3]);

    int height(0), width(0), channel(0);
    int shape[3];
    uint8* image; // = new uint8[700*500*3];

    clock_t t0, t1, t2, t3;
    int rank, size;
    int margin = (window_size-1) / 2;
    MPI_Status stat;
    uint8* buff;
    Job* jobs;

    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    // Load image.
    if (rank == 0) {
        if (window_size % 2 == 0) {
            printf("Please give odd window_size like 3,5,7...\n");
            return 0;
        }
        fullname = std::string(args[1]);
        printf("File name: %s\n", fullname.c_str());
        int namelen = fullname.find_last_of('.');
        name = fullname.substr(0, namelen);
        suffix = fullname.substr(namelen+1, name.length()-namelen-1);
        // nfullname = name + "_medfilt." + suffix;
        nfullname = std::string(args[2]);
        printf("name=%s, suffix=%s, nfullname=%s\n", name.c_str(), suffix.c_str(), nfullname.c_str());
        // getImgSize(fullname, height, width, channel);
        // image = new uint8[height*width*channel];
        readImg(image, height, width, channel, (char*)fullname.c_str());
        shape[0] = height; shape[1] = width; shape[2] = channel;
        printf("Input image size: [height, width, channel] = [%d, %d, %d]\n", height, width, channel);
    }

    MPI_Bcast(shape, 3, MPI_INT, 0, MPI_COMM_WORLD);

    height = shape[0]; width = shape[1]; channel = shape[2];
    // int buffsize2 = (shape[0]+2*margin)*(shape[1]+2*margin)*shape[2];
    // printf("buffsize1=%d\n", buffsize1);
    buff = new uint8[(height+2*margin)*(width+2*margin)*channel];
    // printf("Rank %d buffsize: %d, %d\n", rank, buffsize1, buffsize2);

    // Send job dim: master -> slave
    if (rank == 0) {
        t0 = clock();
        // in master process, jobs contains `size` jobs.
        jobs = new Job[size]; // jobs[0] for master process will not be visited
        jobSchedule2dim(height, width, channel, margin, size, jobs);

        // send schedule
        for (int i=1; i<size; i++) {
            MPI_Send(&jobs[i], sizeof(Job), MPI_BYTE, i, TAG, MPI_COMM_WORLD);
        }
    } else {
        // in slave process, jobs contains a single job.
        jobs = new Job;
        MPI_Recv(jobs, sizeof(Job), MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
        printf("Process %02d received job schedule: h=%d, w=%d, c=%d, padh=%d, padw=%d\n",
            rank, jobs->h, jobs->w, jobs->c, jobs->padh, jobs->padw);
    }


    // Send job buff (image block): master -> slave
    if (rank == 0) {
        t1 = clock();
        // buff = (uint8*)calloc((height+2*margin)*(width+2*margin)*channel, sizeof(uint8)); // allocate a much larger buff
        int buffsize;
        // printf("Before Creating buffer for slave\n");
        printf("image [0]: %c\n", image[0]);
        // saveImg("./binimage/original.bin", height, width, channel, image);
        for (int i=1; i<size; i++) {
            printf("jobs[%d].h=%d\n", i, jobs[i].h);
            if (jobs[i].h == 0) {
                printf("Skipped sending buff to %d\n", i);
                continue;
            }
            createBuff(image, height, width, channel, &jobs[i], buff);
            // if (i == 1) {
            //     saveImg("./binimage/buff.rank021.bin", jobs[i].h+2*jobs[i].padh, jobs[i].w+2*jobs[i].padw,
            //         jobs[i].c, buff);
            // }
            printf("Created buffer for slave %d\n", i);
            buffsize = (jobs[i].h+2*jobs[i].padh)*(jobs[i].w+2*jobs[i].padw)*jobs[i].c;
            printf("Send buffsize: %d\n", buffsize);
            MPI_Send(buff, buffsize, MPI_UINT8_T, i, TAG, MPI_COMM_WORLD);
        }
        // free(buff);
    } else {
        if (jobs->h != 0) {
            int buffsize = (jobs->h+2*jobs->padh)*(jobs->w+2*jobs->padw)*jobs->c;
            // buff = (uint8*)calloc(buffsize, sizeof(uint8));
            MPI_Recv(buff, buffsize, MPI_UINT8_T, 0, TAG, MPI_COMM_WORLD, NULL);
            printf("Receive buffsize: %d\n", buffsize);
            // printf("Slave %d received buffer from master.\n", rank);
            // if (rank == 1) {
            //     saveImg("./binimage/buff.rank1.bin", jobs->h+2*jobs->padh, jobs->w+2*jobs->padw,
            //         jobs->c, buff);
            // }
            medianBlurColoredJob(buff, jobs); // actually the pointer to 1 single job is passed
            // printf("Slave %d finished median filt job.\n", rank);
            MPI_Send(buff, buffsize, MPI_UINT8_T, 0, TAG, MPI_COMM_WORLD);
            printf("Slave %d sent buff back.\n", rank);
        } else {
            printf("Skipped receiving buff at %d\n", rank);
        }

    }
    

    // Send job buff (image block): slave -> master
    if (rank == 0) {
        t2 = clock();
        // buff = new uint8[(height+2*margin)*(width+2*margin)*channel]; // allocate a much larger buff
        int buffsize;
        for (int i=1; i<size; i++) {
            if (jobs[i].h == 0) {
                printf("Skipped receiving buff from %d\n", i);
                continue;
            }
            buffsize = (jobs[i].h+2*jobs[i].padh)*(jobs[i].w+2*jobs[i].padw)*jobs[i].c;
            MPI_Recv(buff, buffsize, MPI_UINT8_T, i, TAG, MPI_COMM_WORLD, &stat);
            registerBuff(image, height, width, channel, &jobs[i], buff);
        }
    }

    delete[] jobs;
    delete[] buff;

    if (rank == 0) {
        t3 = clock();
        printf("Program finished.\n");
        printf("\ttime for job information passing: %f\n s", (double)(t1-t0)/CLOCKS_PER_SEC);
        printf("\ttime for job buffer passing: %f\n s", (double)(t2-t1)/CLOCKS_PER_SEC);
        printf("\ttime for slave running and job aggregation: %f s\n", (double)(t3-t2)/CLOCKS_PER_SEC);
    }

    if (rank == 0) {
        printf("height=%d, width=%d, channel=%d\n", height, width, channel);
        saveImg(image, height, width, channel, nfullname);
    }

    MPI_Finalize();



    delete[] image;
    // free(image);
    return 0;
}