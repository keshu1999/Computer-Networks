#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	PORT	8882
#define	PACKET_SIZE	100
#define	DROP_RATE 	10
#define	RETRANSMIT_TIME	2

#define RESET   "\033[0m"
#define RED     "\033[1m\033[31m"
#define GREEN   "\033[1m\033[32m"
#define YELLOW  "\033[1m\033[33m"
#define BLUE    "\033[1m\033[34m"
#define MAGENTA "\033[1m\033[35m"
#define CYAN    "\033[1m\033[36m"

typedef struct packet
{
	char payload[PACKET_SIZE];
	int payloadsize;
	int SeqNo;
	bool lastPacket;
	bool DATA_ACK;
	bool channel_info;
	int numPackets;
}Packet;

Packet* UnAckedPacks[2];

int socket1, socket2;
int FilePkts;
int Sequence;

FILE *clientLogs, *serverLogs;

timer_t* timer[2];

struct itimerspec timerVal[2];








