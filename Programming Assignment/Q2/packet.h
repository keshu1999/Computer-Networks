#include <stdio.h>
#include <stdarg.h>
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
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CLIENT_PORT 8881
#define SERVER_PORT 8882
#define RELAY0_PORT 8883
#define RELAY1_PORT 8884
#define	PACKET_SIZE	100
#define	DROP_RATE 	10
#define	RETRANSMIT_TIME	2
#define DELAY_TIME	2
#define WINDOW_SIZE 5

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
	int RelayNode;
	int FileOffset;
}Packet;

int FilePkts, Sequence, clientSocket;

Packet* UnAcked[WINDOW_SIZE];

timer_t* timer[WINDOW_SIZE];

struct itimerspec timerValue[2];

FILE *ClientLogs, *Relay0Logs, *Relay1Logs, *ServerLogs, *MainLog;

