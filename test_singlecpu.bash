

for scheduledim in 2
do
    resultfile=./cache/result.single_cpu.$[$scheduledim]dimschedule.txt
    rm $resultfile
    for fileid in 1 2 3 4 5 6 7 8 9 10 11 12 13
    do 
        for window_size in 3 5 7 9 11 13 15
        do
            filename=./image/$[$fileid].jpg
            sfilename=./image/$[$fileid]_medfilt.$[$window_size].jpg
            LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"./" ./singlecpu.o $filename $sfilename $window_size $resultfile
            echo Finished medfilt for file $fileid with window_size=$window_size
        done
        rm core.*
    done
done