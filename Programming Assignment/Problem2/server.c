#include "packet.h" 

void die(char *message)
{
	perror(message);
	exit(1);
}


void PrintPacket(Packet* pack)
{
	fprintf(ServerLogs, "*******************************\n");
	fprintf(ServerLogs, "PAYLOAD : 		%s\n", pack->payload);
	fprintf(ServerLogs, "SIZE : 			%d\n", pack->payloadsize);
	fprintf(ServerLogs, "SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(ServerLogs, "LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(ServerLogs, "DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(ServerLogs, "RELAY NODE : 		%d\n", pack->RelayNode);
	fprintf(ServerLogs, "*******************************\n\n\n");
	return;
}


bool Relay0Or1()
{
	return rand()%2;	
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
	printf("SERVER\t\t%s\t%s\t\t%s\t\t%d\t\t%s\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
	FilePrint(" SERVER\t\t\t%s\t\t\t%s\t\t\t%s\t\t\t%d\t\t\t%s\t\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
}



Packet* ConstructAckPacket(Packet* receivedPack, Packet* ackpack)
{
	sprintf(ackpack->payload, "Acknowledgement for Packet Number %d", receivedPack->SeqNo);	
	ackpack->payloadsize = sizeof(ackpack->payload);
        ackpack->SeqNo = receivedPack->SeqNo;
        ackpack->lastPacket = receivedPack->lastPacket;
        ackpack->DATA_ACK = 1;
        ackpack->RelayNode = receivedPack->RelayNode;
    	return ackpack;
}


int main()
{
	srand(time(0));
	int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);  
	if(serverSocket< 0)
		die("ERROR : Making Server Socket\n");
	
	struct sockaddr_in serverAddr, relayAddr;
	memset(&serverAddr, 0, sizeof(serverAddr)); 
	memset(&relayAddr, 0, sizeof(relayAddr)); 
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT); 
	serverAddr.sin_addr.s_addr = INADDR_ANY; 

	relayAddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &relayAddr.sin_addr);

	if(bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) 
		die("ERROR : Binding Server\n");
	
	int recvStat, relayLen = sizeof(relayAddr);
	
	Packet *RecvPacket = (Packet *)malloc(sizeof(Packet)), *SentPacket = (Packet *)malloc(sizeof(Packet));

	FILE *fp = fopen("output.txt", "w");
	if(fp==NULL)
		die("ERROR : Opening file output.txt\n");

	ServerLogs = fopen("ServerLogs.txt", "w");
	if(ServerLogs == NULL)
		die("ERROR : Opening file logs\n");
		
	MainLog = fopen("Logs.txt", "w");
	if(MainLog == NULL)
		die("ERROR : Opening Main Logs\n");
	FilePrint(" NODE NAME\t\tEVENT TYPE\t\tTIMESTAMP\t\t\tPACKET TYPE\t\tSEQUENCE NUMBER\t\tSOURCE NODE\t\tDESTINATION NODE\n");
	for(int i=0; i<169; i++)
		FilePrint("_");
	FilePrint("\n");
	fclose(MainLog);

	int fpfd = fileno(fp);
	while(1)
	{
		recvStat = recvfrom(serverSocket, RecvPacket, sizeof(Packet), MSG_WAITALL, (struct sockaddr *) &relayAddr, &relayLen); 
		if(recvStat < 1)
			die("ERROR : Receiving Packet\n");
		 
		fprintf(ServerLogs, "Recieved...\n");
		pwrite(fpfd, RecvPacket->payload, RecvPacket->payloadsize, RecvPacket->SeqNo*PACKET_SIZE);
		printf(GREEN);
		if(RecvPacket->RelayNode==0)
			PrintLog("R", RecvPacket, "RELAY1", "SERVER");
		else
			PrintLog("R", RecvPacket, "RELAY2", "SERVER");
		printf(RESET);
		PrintPacket(RecvPacket);
		SentPacket = ConstructAckPacket(RecvPacket, SentPacket);
		if(!Relay0Or1())
		{
			relayAddr.sin_port = htons(RELAY0_PORT);
			SentPacket->RelayNode = 0;
		}
		else
		{
			relayAddr.sin_port = htons(RELAY1_PORT);
			SentPacket->RelayNode = 1;
		}		
		fprintf(ServerLogs, "Sending ACK...\n");
		if(sendto(serverSocket, SentPacket, sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &relayAddr, relayLen) == -1)
			die("ERROR : Couldn't Send Acknowledgement Packet\n");
		printf(BLUE);
		if(SentPacket->RelayNode==0)
			PrintLog("S", SentPacket, "SERVER", "RELAY1");
		else
			PrintLog("S", SentPacket, "SERVER", "RELAY2");
		printf(RESET);		
		PrintPacket(SentPacket);
	}
	close(serverSocket);
	fclose(fp);
	fclose(ServerLogs);
	return 0;
}
