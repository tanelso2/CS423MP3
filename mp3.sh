sudo rmmod mp3
rm -f node 
make 
sudo insmod mp3.ko
sudo mknod node c 251 0
nice ./work 1024 R 50000 & nice ./work 1024 R 10000 &
