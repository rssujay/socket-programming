/*******************************
client.c: the source file of the assignment client used in udp file transfer 
********************************/

#include "headsock.h"

void send_file_over_udp(FILE *fp, int sockfd, struct sockaddr *addr, socklen_t addrlen);	// transmission function
void tv_sub(struct  timeval *out, struct timeval *in);										// calculate the time interval between out and in


int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	// check argument count
	if (argc != 2) {
		printf("parameters not match\n");
	}

	//get host's information
	sh = gethostbyname(argv[1]);
	if (sh == NULL) {
		printf("error when gethostby name\n");
		exit(EXIT_FAILURE);
	}

	//print the remote host's information
	printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype) {
		case AF_INET:
			printf("AF_INET\n");
			break;
		default:
			printf("unknown addrtype\n");
			break;
	}
    
	// extract addresses from host information
	addrs = (struct in_addr **)sh->h_addr_list;
	
	// create a UDP socket and check for errors
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("Error occurred during socket creation\n");
		exit(EXIT_FAILURE);
	}

	// configure parameters over the created socket
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	
	// load file into memory
	if((fp = fopen(FILENAME, "r+t")) == NULL) {
		printf("File doesn't exist\n");
		exit(EXIT_FAILURE);
	}

	send_file_over_udp(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in));

	close(sockfd);
	fclose(fp);
	exit(EXIT_SUCCESS);
}


void send_file_over_udp(FILE *fp, int sockfd, struct sockaddr *addr, socklen_t addrlen) {
	char *buf;
	long file_size;

	// Find length of file
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	printf("The file length is %d bytes\n", (int)file_size);
	printf("the packet length is %d bytes\n",DATALEN);


	// allocate sufficient space to hold entire file
	buf = (char *) malloc(file_size + 1);
	if (buf == NULL) {
		exit(EXIT_FAILURE); // mem allocation fail
	}

	// copy the entire file into the allocated buffer
	fread(buf, 1, file_size, fp);
	buf[file_size] = FILE_END;

	// Prepare performance analysis variables
	struct timeval begin, end;
	float time_inv = 0.0;

	gettimeofday(&begin, NULL);


	// Begin data transmission
	int ack_tracker = 1;
	int since_last_ack = 0;
	int err;
	long curr_idx = 0;
	long send_length;
	char send_buffer[DATALEN];
	struct ack_so ack;

	while (curr_idx <= file_size) {
		// packet data length should be min of (DATALEN, packets left)
		if (file_size + 1 - curr_idx <= DATALEN) {
			send_length = file_size + 1 - curr_idx;
		} else {
			send_length = DATALEN;
		}

		// send out data packet
		since_last_ack++;
		memcpy(send_buffer, (buf + curr_idx), send_length);

		err = sendto(sockfd, &send_buffer, send_length, COMM_FLAGS, addr, addrlen);
		if (err == -1) {
			printf("Error occurred while sending data\n");
			exit(EXIT_FAILURE);
		} else if (DEBUG_MODE) {
			printf("Sent packet: curr_idx=%ld, ack_tracker=%d, since_last_ack=%d\n", curr_idx, ack_tracker, since_last_ack);
		}

		// post-process after each packet
		curr_idx += send_length;

		// check if an acknowledgement is due. If so, wait for one.
		if (STOP_AND_WAIT || ack_tracker == since_last_ack) {
			if ((err = recvfrom(sockfd, &ack, ACK_READ_SIZE, COMM_FLAGS, addr, &addrlen)) == -1) {
				printf("Error occurred while receiving data\n");
				exit(EXIT_FAILURE);
			} else if ((ack.num != ACK_NUM) | (ack.len != ACK_LEN)) {
				printf("Error occurred when receiving ack\n");
			} else if (DEBUG_MODE) {
				printf("Received ack: curr_idx=%ld\n", curr_idx);
			}

			// update ack tracker
			ack_tracker++;
			if (ack_tracker > 3) {
				ack_tracker -= 3;
			}

			// reset ticks since last ack received
			since_last_ack = 0;
		}
	}

	gettimeofday(&end, NULL);
	
	// summarize time taken for transmission
	tv_sub(&end, &begin);
	time_inv += (end.tv_sec)*1000.0 + (end.tv_usec)/1000.0;

	printf("Time taken for transmission (ms) : %.3f\n", time_inv);
	printf("Data sent (bytes) : %ld\n", curr_idx);
	printf("Data rate (Kbyte/s) : %.3f\n", (float)curr_idx/time_inv);
}


void tv_sub(struct  timeval *out, struct timeval *in) {
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
