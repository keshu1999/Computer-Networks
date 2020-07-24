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
	printf("RELAY2\t\t%s\t%s\t\t%s\t\t%d\t\t%s\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
	FilePrint(" RELAY2\t\t\t%s\t\t\t%s\t\t\t%s\t\t\t%d\t\t\t%s\t\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
}



bool DropOrSend()
{
	if(rand()%100 < DROP_RATE)
		return 0;
	return 1;
}


void PrintPacket(Packet* pack)
{
	fprintf(Relay1Logs, "*******************************\n");
	fprintf(Relay1Logs, "PAYLOAD : 		%s\n", pack->payload);
	fprintf(Relay1Logs, "SIZE : 			%d\n", pack->payloadsize);
	fprintf(Relay1Logs, "SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(Relay1Logs, "LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(Relay1Logs, "DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(Relay1Logs, "RELAY NODE : 		%d\n", pack->RelayNode);
	fprintf(Relay1Logs, "*******************************\n\n\n");
	return;
}


int main()
{
	srand(time(0));
	int relay1Socket = socket(AF_INET, SOCK_DGRAM, 0);  
	if(relay1Socket < 0)
		die("ERROR : Couldn't Connect to Socket\n");

	struct sockaddr_in relay1Addr, receiveAddr; 
	memset(&relay1Addr, 0, sizeof(relay1Addr)); 
	memset(&receiveAddr, 0, sizeof(receiveAddr)); 
	
	relay1Addr.sin_family = AF_INET;
	relay1Addr.sin_port = htons(RELAY1_PORT);
	relay1Addr.sin_addr.s_addr = INADDR_ANY; 

	if(bind(relay1Socket, (struct sockaddr *) &relay1Addr, sizeof(relay1Addr))< 0) 
		die("ERROR : Binding Relay Node 1\n");

	Relay1Logs = fopen("Relay2Logs.txt", "w");
	if(Relay1Logs == NULL)
		die("ERROR : Opening file logs\n");

	int recvStat, recvLen = sizeof(receiveAddr);
	
	Packet* relay1Receive = (Packet *)malloc(sizeof(Packet));
	while(1)
	{
		recvStat = recvfrom(relay1Socket, relay1Receive, sizeof(Packet), MSG_WAITALL, (struct sockaddr *) &receiveAddr, &recvLen); 
		if(recvStat < 1)
			die("ERROR : Receiving Packet\n");
		if(DropOrSend()||relay1Receive->DATA_ACK)							// Packet Not Dropped		
		{		
			fprintf(Relay1Logs, "Received Packet...\n");
			printf(GREEN);
			PrintPacket(relay1Receive);
			if(relay1Receive->DATA_ACK == 1)
			{
				receiveAddr.sin_port = htons(CLIENT_PORT);
				PrintLog("R", relay1Receive, "SERVER", "RELAY2");
				fprintf(Relay1Logs, "Forwarding Packet to Client...\n");
				if(sendto(relay1Socket, relay1Receive, sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &receiveAddr, recvLen) == -1)
					die("ERROR : Couldn't Send Acknowledgement Packet\n");
				printf(BLUE);
				PrintLog("S", relay1Receive, "RELAY2", "CLIENT");
				PrintPacket(relay1Receive);
			}
			else
			{
				receiveAddr.sin_port = htons(SERVER_PORT);
				usleep(Delay());
				PrintLog("R", relay1Receive, "CLIENT", "RELAY2");
				fprintf(Relay1Logs, "Forwarding Packet to server...\n");
				if(sendto(relay1Socket, relay1Receive, sizeof(Packet), MSG_CONFIRM, (const struct sockaddr *) &receiveAddr, recvLen) == -1)
					die("ERROR : Sending Packet to Server\n");
				printf(BLUE);
				PrintLog("S", relay1Receive, "RELAY2", "SERVER"); 
				PrintPacket(relay1Receive);
			}
		}
		else
		{
			fprintf(Relay1Logs, "Dropping Packet from Client...\n");
			printf(RED);			
			PrintLog("D", relay1Receive, "CLIENT", "RELAY2");
			PrintPacket(relay1Receive);
			continue;
		}
	}
	close(relay1Socket);
	fclose(Relay1Logs);	
	return 0;
}
