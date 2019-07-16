target: mpi.o
	mpicxx libjpeg.so medianBlurColored_mpi.cpp -I ./libjpeg-build/include -L ./libjpeg-build/lib -o mpi.o