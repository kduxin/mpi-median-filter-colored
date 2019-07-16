mpi.o:
	mpicxx -O3 medianBlurColored_mpi.cpp ~/libs/libjpeg/lib/libjpeg.so  -I ~/libs/libjpeg/include -L ~/libs/libjpeg/lib -o mpi.1dimschedule.o