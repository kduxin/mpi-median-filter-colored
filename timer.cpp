#include <time.h>

using namespace std;


double diff(timespec start, timespec end) // resolution: 1 milisecond
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    
    return double(temp.tv_sec)*1000 + double(temp.tv_nsec)/1000000;
}