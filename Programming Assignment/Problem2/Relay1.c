#include "packet.h" 

void die(char *message)
{
	perror(message);
	exit(1);
}


int Delay()
{
	return rand()%(DELAY_TIME*1000);	
}


char* GetCurrentTime()
{
	char *tim = (char *)malloc(sizeof(char)*20);
	int rc;
	time_t curr;
	struct tm* timeptr;
	struct timeval tv;
	
	curr = time(NULL);
	timeptr = localtime(&curr);
	
	gettimeofday(&tv, NULL);

	rc = strftime(tim, 20, "%H:%M:%S", timeptr);

	char ms[8];
	sprintf(ms, ".%06ld", tv.tv_usec);
	strcat(tim, ms);
	return tim;
}


void FilePrint(char *format, ...)
{
	MainLog = fopen("Logs.txt", "a+");
	va_list ap;
	va_start(ap, format);
	int over = vfprintf(MainLog, format, ap);
	va_end(ap);
	fclose(MainLog);
}


void PrintLog(char *Event, Packet* pkt,  char* source, char* dest)
{
	printf("RELAY1\t\t%s\t%s\t\t%s\t\t%d\t\t%s\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
	FilePrint(" RELAY1\t\t\t%s\t\t\t%s\t\t\t%s\t\t\t%d\t\t\t%s\t\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
}



bool DropOrSend()
{
	if(rand()%100 < DROP_RATE)
		return 0;
	return 1;
}


void PrintPacket(Packet* pack)
{
	fprintf(Relay0Logs, "*******************************\n");
	fprintf(Relay0Logs, "PAYLOAD : 		%s\n", pack->payload);
	fprintf(Relay0Logs, "SIZE : 			%d\n", pack->payloadsize);
	fprintf(Relay0Logs, "SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(Relay0Logs, "LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(Relay0Logs, "DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(Relay0Logs, "RELAY NODE : 		%d\n", pack->RelayNode);
	fprintf(Relay0Logs, "*******************************\n\n\n");
	return;
}


int main()
{
	srand(time(0));
	int relay0Socket = socket(AF_INET, SOCK_DGRAM, 0);  
	if(relay0Socket < 0)
		die("ERROR : Couldn't Connect to Socket\n");

	struct sockaddr_in relay0Addr, receiveAddr; 
	memset(&relay0Addr, 0, sizeof(relay0Addr)); 
	memset(&receiveAddr, 0, sizeof(receiveAddr)); 
	
	relay0Addr.sin_family = AF_INET;
	relay0Addr.sin_port = htons(RELAY0_PORT);
	relay0Addr.sin_addr.s_addr = INADDR_ANY; 

	if(bind(relay0Socket, (struct sockaddr *) &relay0Addr, sizeof(relay0Addr))< 0) 
		die("ERROR : Binding Relay Node 0\n");

	Relay0Logs = fopen("Relay1Logs.txt", "w");
	if(Relay0Logs == NULL)
		die("ERROR : Opening file logs\n");

	int recvStat, recvLen = sizeof(receiveAddr);
	
	Packet* relay0Receive = (Packet *)malloc(sizeof(Packet));
	while(1)
	{
		recvStat = recvfrom(relay0Socket, relay0Receive, sizeof(Packet), MSG_WAITALL, (struct sockaddr *) &receiveAddr, &recvLen); 
		if(recvStat < 1)
			die("ERROR : Receiving Packet\n");
		if(DropOrSend()||relay0Receive->DATA_ACK)							// Packet Not Dropped
		{
			fprintf(Relay0Logs, "Received Packet...\n");
			printf(GREEN);
			PrintPacket(relay0Receive);
			if(relay0Receive->DATA_ACK == 1)
			{
				receiveAddr.sin_port = htons(CLIENT_PORT);
				PrintLog("R", relay0Receive, "SERVER", "RELAY1");
				fprintf(Relay0Logs, "Forwarding Packet to client...\n");
				if(sendto(relay0Socket, relay0Receive, sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &receiveAddr, recvLen) == -1)
					die("ERROR : Couldn't Send Acknowledgement Packet\n");
				printf(BLUE);
				PrintLog("S", relay0Receive, "RELAY1", "CLIENT");
				PrintPacket(relay0Receive);
			}
			else
			{
				receiveAddr.sin_port = htons(SERVER_PORT);
				usleep(Delay());
				PrintLog("R", relay0Receive, "CLIENT", "RELAY1");
				fprintf(Relay0Logs, "Forwarding Packet to server...\n");
				if(sendto(relay0Socket, relay0Receive, sizeof(Packet), MSG_CONFIRM, (const struct sockaddr *) &receiveAddr, recvLen) == -1)
					die("ERROR : Sending Packet to Server\n");
				printf(BLUE);
				PrintLog("S", relay0Receive, "RELAY1", "SERVER"); 
				PrintPacket(relay0Receive);
			}
			printf(RESET);
		}
		else
		{
			fprintf(Relay0Logs, "Dropping Packet....\n");
			printf(RED);			
			PrintLog("D", relay0Receive, "CLIENT", "RELAY1");
			PrintPacket(relay0Receive);
			continue;
		}
	}
	close(relay0Socket);
	fclose(Relay0Logs);
	return 0;
}
