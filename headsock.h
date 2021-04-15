// headfile for TCP program
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#define NEWFILE (O_WRONLY|O_CREAT|O_TRUNC)
#define MYTCP_PORT 4950
#define MYUDP_PORT 5351
#define DATALEN 10
#define BUFSIZE 310000
#define PACKLEN 508
#define HEADLEN 8
#define ACK_NUM 1
#define ACK_LEN 0
#define FILENAME "myfile_short.txt"
#define STOP_AND_WAIT 1     // 1 indicates stop and wait is used, else if 0, 1->2->3 ack is used
#define COMM_FLAGS 0
#define ACK_READ_SIZE 2
#define FILE_END '\0'
#define DEBUG_MODE 1
#define TIMEOUT_MS 500
#define MAX_ACK_RESENDS 10


//data packet structure
struct pack_so {
    uint32_t num;		    // the sequence number
    uint32_t len;		    // the packet length
    char data[DATALEN];	    // the packet data
};

struct ack_so {
    uint8_t num;
    uint8_t len;
};
