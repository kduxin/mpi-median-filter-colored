mpi.o:
	mpic++ medianBlurColored_mpi.cpp ~/libs/libjpeg/lib/libjpeg.so  -I ~/libs/libjpeg/include -L ~/libs/libjpeg/lib -o mpi.o