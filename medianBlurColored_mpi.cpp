
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <math.h>
#include <memory>
#include <time.h>
#include <mpi.h>

#define TAG 0
#define min(a,b) (a<b?a:b)

typedef unsigned char uint8;

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

    int height, width, channel;
    int padh, padw;
    padh = job->padh; padw = job->padw;
    height = job->h+2*padh; width = job->w+2*padw; channel = job->c;

    int i(0);
    int cachelen = (2*padh+1)*(2*padw+1);
    uint8* cache = new uint8[(2*padh+1)*(2*padw+1)];
    uint8* image_o = new uint8[height*width*channel];
    for (int c = 0; c < channel; c++) {
        for (int h = padh; h < height-padh; h++) {
            for (int w = padw; w < width-padw; w++) {
                i = 0;
                for (int p = h-padh; p < h+padh+1; p++) {
                    for (int q = w-padw; q < w+padw+1; q++) {
                        cache[i] = image[c+(p*width+q)*channel];
                        ++i;
                    }
                }
                quickSort(cache, 0, cachelen-1);
                image_o[c+(h*width+w)*channel] = cache[cachelen/2];
            }
        }
    }
    memcpy(image, image_o, sizeof(uint8)*height*width*channel);
    for (int i = 0; i < height*width*channel; i++) {
        image[i] = image_o[i];
    }
    delete[] image_o;
    
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
            jobs[i].padh = margin;
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

}

void createBuff(uint8* image, int height, int width, int channel, Job* job, uint8* buff) {
    // TODO
    int h, w, c, ystart, xstart, cstart, padh, padw;
    int blockw, blockh;
    h=job->h; w=job->w; c=job->c;
    ystart=job->ystart; xstart=job->xstart; cstart=job->cstart;
    padh=job->padh; padw=job->padw;
    blockw = w+2*padw; blockh = h+2*padh;

    for (int y=ystart-padh; y<ystart+h+padh; y++) {
        for (int x=xstart-padw; x<xstart+h+padw; x++) {
            for (int z=cstart; z<cstart+c; z++) {
                if (y<0 || y>=height || x<0 || x>=width) {
                    buff[c*(blockw*(y-ystart+padh)+x-xstart+padw)
                                +z-cstart] = image[channel*(width*y+x)+z];
                } else {
                    buff[c*(blockw*(y-ystart+padh)+x-xstart+padw)
                                +z-cstart] = 0;
                }
            }
        }
    }
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

extern "C" float medianBlurColored_MPI(uint8* image, int height, int width, int channel, int window_size) {
    clock_t t0, t1, t2, t3;
    int rank, size;
    MPI_Status stat;
    uint8* buff;
    Job* jobs;
    int margin = (window_size-1) / 2;

    if (window_size % 2 == 0) {
        printf("Please give odd window_size like 3,5,7...\n");
        return 0;
    }
    printf("Input image size: [height, width, channel] = [%d, %d, %d]\n", height, width, channel);
    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Send job dim: master -> slave
    if (rank == 0) {
        t0 = clock();
        // in master process, jobs contains `size` jobs.
        jobs = new Job[size]; // jobs[0] for master process will not be visited
        jobSchedule1dim(height, width, channel, margin, size, jobs);

        // send schedule
        for (int i=1; i<size; i++) {
            MPI_Send(&jobs[i], sizeof(Job), MPI_BYTE, i, TAG, MPI_COMM_WORLD);
        }
    } else {
        // in slave process, jobs contains a single job.
        jobs = new Job;
        MPI_Recv(&jobs, sizeof(Job), MPI_BYTE, 0, MPI_INT, MPI_COMM_WORLD, &stat);
        printf("Process %02d received job schedule: h=%d, w=%d, c=%d, padh=%d, padw=%d\n",
            rank, jobs->h, jobs->w, jobs->c, jobs->padh, jobs->padw);
    }


    // Send job buff (image block): master -> slave
    if (rank == 0) {
        t1 = clock();
        buff = new uint8[(height+margin)*(width+margin)*channel]; // allocate a much larger buff
        int buffsize;
        for (int i=1; i<size; i++) {
            createBuff(image, height, width, channel, &jobs[i], buff);
            buffsize = (jobs[i].h+jobs[i].padh)*(jobs[i].w+jobs[i].padw)*jobs[i].c;
            MPI_Send(buff, buffsize, MPI_UINT8_T, i, TAG, MPI_COMM_WORLD);
        }
        delete[] buff;
    } else {
        int buffsize = (jobs->h+jobs->padh)*(jobs->w+jobs->padw)*jobs->c;
        buff = new uint8[buffsize];
        MPI_Recv(buff, buffsize, MPI_UINT8_T, 0, TAG, MPI_COMM_WORLD, &stat);
        medianBlurColoredJob(buff, jobs); // actually the pointer to 1 single job is passed
        MPI_Send(buff, buffsize, MPI_UINT8_T, 0, TAG, MPI_COMM_WORLD);
    }

    // Send job buff (image block): slave -> master
    if (rank == 0) {
        t2 = clock();
        buff = new uint8[(height+margin)*(width+margin)*channel]; // allocate a much larger buff
        int buffsize;
        for (int i=1; i<size; i++) {
            buffsize = (jobs[i].h+jobs[i].padh)*(jobs[i].w+jobs[i].padw)*jobs[i].c;
            MPI_Recv(buff, buffsize, MPI_UINT8_T, i, TAG, MPI_COMM_WORLD, &stat);
            registerBuff(image, height, width, channel, &jobs[i], buff);
        }
    }

    delete[] jobs;

    if (rank == 0) {
        t3 = clock();
        printf("Program finished.\n");
        printf("\ttime for job information passing: %f\n", t1-t0);
        printf("\ttime for job buffer passing: %f\n", t2-t1);
        printf("\ttime for slave running and job aggregation: %f\n", t3-t2);
    }

    MPI_Finalize();


}



// compile: g++ medianBlurColored.cpp -fPIC -shared -o lib_cpu.so
