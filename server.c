/**********************************
server.c: the source file of the server in udp file transfer
***********************************/

#include "headsock.h"

int receive_file_over_udp(int sockfd);
void send_ack_over_udp(int sockfd, struct sockaddr *addr, socklen_t addrlen, long curr_idx);

int main(void) {
	int sockfd;
	struct sockaddr_in my_addr;

	// create socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Error occurred during socket creation\n");
		exit(EXIT_FAILURE);
	}
	
	// configure parameters over the created socket
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = TIMEOUT_MS * 1000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		printf("Error occurred when setting socket timeout.\n");
		exit(EXIT_FAILURE);
	}

	// bind socket to run a udp server
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		printf("Error occurred during binding.\n");
		exit(EXIT_FAILURE);
	}

	printf("Receiving file\n");
	receive_file_over_udp(sockfd);

	close(sockfd);
	exit(EXIT_SUCCESS);
}


int receive_file_over_udp(int sockfd) {
	char buf[BUFSIZE];
	FILE *fp;
	
	char recv_buffer[DATALEN];
	
	int end = 0;
	int bytes_recvd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	long curr_idx;

	int ack_tracker = 1;
	int since_last_ack = 0;
	int acks_resent = 0;

	struct sockaddr_in addr;

	// Receive file
	while(end == 0) {

		// Under normal operation, this inner while loop only executes once
		while ((bytes_recvd = recvfrom(sockfd, &recv_buffer, DATALEN, COMM_FLAGS, (struct sockaddr *) &addr, &addrlen)) == -1) {
			// since we use UDP, packets received and acks sent out can be lost halfway
			// hence we use a while loop here to resend acks in case this happens
			if (curr_idx > 0) {
				if (acks_resent > MAX_ACK_RESENDS) {
					printf("Max number of retries exceeded. Server terminating...\n");
					exit(EXIT_FAILURE);
				}

				printf("Resending acknowledgement. \n");
				send_ack_over_udp(sockfd, (struct sockaddr *) &addr, addrlen, curr_idx);
				acks_resent++;
				since_last_ack = 0; // resynchronize with sender
			}
		}

		if (DEBUG_MODE) {
			printf("Received packet: curr_idx=%ld, ack_tracker=%d, since_last_ack=%d\n", curr_idx, ack_tracker, since_last_ack);
		}
		
		if (recv_buffer[bytes_recvd - 1] == FILE_END) {
			end = 1;
			bytes_recvd--;
		}

		// post-process after each packet
		memcpy((buf + curr_idx), recv_buffer, bytes_recvd);
		curr_idx += bytes_recvd;
		since_last_ack++;
		acks_resent = 0;

		// if an acknowledgement is due, send one out
		if (STOP_AND_WAIT || ack_tracker == since_last_ack) {
			// send out an ack
			send_ack_over_udp(sockfd, (struct sockaddr *) &addr, addrlen, curr_idx);

			// update ack tracker
			ack_tracker++;
			if (ack_tracker > 3) {
				ack_tracker -= 3;
			}

			// reset ticks since last ack received
			since_last_ack = 0;
		}
	}

	// Write buffer out to file
	if ((fp = fopen("myUDPreceive.txt","wt")) == NULL) {
		printf("File cannot be created.\n");
		exit(EXIT_FAILURE);
	}
	
	fwrite(buf, 1, curr_idx, fp);
	fclose(fp);

	printf("File received and stored. Size : %ld (bytes)\n", curr_idx);

	return 1;
}


void send_ack_over_udp(int sockfd, struct sockaddr *addr, socklen_t addrlen, long curr_idx) {
	int err;

	struct ack_so ack;
	ack.num = ACK_NUM;
	ack.len = ACK_LEN;

	err = sendto(sockfd, &ack, 2, COMM_FLAGS, addr, addrlen);
	if (err == -1) {
		printf("Error occurred while sending ack\n");
	} else if (DEBUG_MODE) {
		printf("Sent ack: curr_idx=%ld\n", curr_idx);
	}
}
