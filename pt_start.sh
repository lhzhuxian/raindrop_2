#!/bin/bash
echo "16x16"
./rainfall_pt 2 20 0.5 16 sample_16x16.in > ans.txt
diff sample_16x16.out ans.txt
echo "32x32"
./rainfall_pt 2 20 0.5 32 sample_32x32.in > ans.txt
diff sample_32x32.out ans.txt
echo "128x128"
./rainfall_pt 2 30 0.25 128 sample_128x128.in > ans.txt
diff sample_128x128.out ans.txt
echo "512x512"
./rainfall_pt 2 30 0.75 512 sample_512x512.in > ans.txt
diff sample_512x512.out ans.txt
echo "2048x2048"
./rainfall_pt 2 35 0.5 2048 sample_2048x2048.in > ans.txt
./check.py 2048 ./ans.txt ./sample_2048x2048.out
echo "4096x4096"
./rainfall_pt 2 50 0.5 4096 measurement_4096x4096.in > ans.txt
./check.py 2048 ./ans.txt ./measurement_4096x4096.out
