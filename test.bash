#!/bin/bash
#PBS -q u-lecture
#PBS -W group_list=gt24
#PBS -l select=1:mpiprocs=20:ompthreads=1
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
. /etc/profile.d/modules.sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"./" mpiexec ./mpi.o /lustre/gt24/t24040/code/2019s1s2report/image/aimer.jpg /lustre/gt24/t24040/code/2019s1s2report/image/aimer.medfilt.5.jpg 3
