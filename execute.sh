gcc -Wall -o client client.c
gcc -Wall -o server server.c

./server &
./client 127.0.0.1 &
wait

diff myfile_short.txt myUDPreceive.txt