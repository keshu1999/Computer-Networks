#include "packet.h" 

void die(char *message)
{
	perror(message);
	exit(1);
}


void PrintPacket(Packet* pack)
{
	fprintf(ClientLogs, "*******************************\n");
	fprintf(ClientLogs, "PAYLOAD : 		%s\n", pack->payload);
	fprintf(ClientLogs, "SIZE : 			%d\n", pack->payloadsize);
	fprintf(ClientLogs, "SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(ClientLogs, "LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(ClientLogs, "DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(ClientLogs, "RELAY NODE : 		%d\n", pack->RelayNode);
	fprintf(ClientLogs, "*******************************\n\n\n");
	return;
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
	printf("CLIENT\t\t%s\t%s\t\t%s\t\t%d\t\t%s\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
	FilePrint(" CLIENT\t\t\t%s\t\t\t%s\t\t\t%s\t\t\t%d\t\t\t%s\t\t\t%s\n", Event, GetCurrentTime(), pkt->DATA_ACK==0?"DATA":"ACK", pkt->SeqNo, source, dest);
}



int NoofChars(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if(fp==NULL)
		die("ERROR : Reading from file input.txt\n");
	char c;
	int count = 0;
	while((c=fgetc(fp))!=EOF)
		count++;
	return count;
}


void CreateTimers()
{
	struct sigevent events[WINDOW_SIZE];
	for(int i=0; i<WINDOW_SIZE; i++)
	{
		timer[i] = (timer_t*)malloc(sizeof(timer_t));
		events[i].sigev_notify = SIGEV_SIGNAL;
		events[i].sigev_signo = SIGALRM;
		events[i].sigev_value.sival_ptr = timer[i];
		timer_create(CLOCK_REALTIME, &events[i], timer[i]);
		timerValue[i].it_interval.tv_sec = 0;
		timerValue[i].it_interval.tv_nsec = 0;
		timerValue[i].it_value.tv_sec = RETRANSMIT_TIME;
		timerValue[i].it_value.tv_nsec = 0;
	}
	return;	
}


Packet* ConstructPacket(FILE* fp, Packet* pack)
{
	int bytesRead = fread(pack->payload, 1, PACKET_SIZE, fp);
	if(bytesRead == PACKET_SIZE)
	{
		pack->payloadsize = PACKET_SIZE;
		pack->SeqNo = Sequence;
		pack->lastPacket = 0;
		pack->DATA_ACK = 0;
		pack->RelayNode = Sequence % 2;
		pack->FileOffset = Sequence*PACKET_SIZE;
	}
	else
	{
		if(!feof(fp))
			die("ERROR : Reading from file input.txt\n");
		else
		{
			pack->payloadsize = bytesRead;
			pack->SeqNo = Sequence;
			pack->lastPacket = 1;
			pack->DATA_ACK = 0;
			pack->RelayNode = Sequence % 2;
			pack->FileOffset = Sequence*PACKET_SIZE;
		}
	}
	Sequence++;
	return pack;
}


static void SignalHandler(int signal, siginfo_t *siginfo, void *uc)
{
	timer_t *temptime;
	temptime = siginfo->si_value.sival_ptr;
	struct sockaddr_in DestinAddr;
	DestinAddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &DestinAddr.sin_addr);
	int i;	
	for(i=0; i<WINDOW_SIZE; i++)
	{
		if(*temptime == *timer[i])
		{
			fprintf(ClientLogs, "Resending Packet...\n");
			printf(RED);
			if(UnAcked[i]->RelayNode==0)			
			{
				PrintLog("TO", UnAcked[i], "CLIENT", "RELAY1");
				DestinAddr.sin_port = htons(RELAY0_PORT);
				printf(YELLOW);
				PrintLog("RE", UnAcked[i], "CLIENT", "RELAY1");
			}			
			else
			{
				PrintLog("TO", UnAcked[i], "CLIENT", "RELAY2");
				DestinAddr.sin_port = htons(RELAY1_PORT);
				printf(YELLOW);
				PrintLog("RE", UnAcked[i], "CLIENT", "RELAY2");			
			}
			printf(RESET);			
			int tempResend = sendto(clientSocket, UnAcked[i], sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &DestinAddr, sizeof(DestinAddr));
			if(tempResend < 0)
				die("Couldn't Resend Packet\n");
			if(timer_settime(*timer[i], 0, &timerValue[i], NULL) == -1)
				die("ERROR : Resetting Timer\n");
			PrintPacket(UnAcked[i]);
			break;
		}
	}
	if(i==WINDOW_SIZE)
		die("ERROR : Timer Malfunctioned\n");
	return;
}



int main()
{
	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);  
	if(clientSocket < 0)
		die("ERROR : Couldn't Connect to Socket\n");
	
	struct sockaddr_in clientAddr, receiverAddr; 
	memset(&clientAddr, 0, sizeof(clientAddr)); 
	memset(&receiverAddr, 0, sizeof(receiverAddr)); 
	
	receiverAddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &receiverAddr.sin_addr);	

	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(CLIENT_PORT);
	clientAddr.sin_addr.s_addr = INADDR_ANY; 

	if(bind(clientSocket, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) 
		die("ERROR : Binding Client\n");

	int AddrLen = sizeof(receiverAddr), recvStat;

	ClientLogs = fopen("ClientLogs.txt", "w");
	if(ClientLogs == NULL)
		die("ERROR : Opening file logs\n");
	
	FILE* fp = fopen("input.txt", "r");
	if(fp == NULL)
		die("ERROR : Opening file input.txt\n");

	Sequence = 0;
    	int filesize = NoofChars("input.txt");
	FilePkts = ceil((double)(filesize)/(PACKET_SIZE));
	
	Packet *RecvPacket = (Packet *)malloc(sizeof(Packet)), *SentPacket = (Packet *)malloc(sizeof(Packet));

	for(int i=0 ; i<WINDOW_SIZE ; i++)
    		UnAcked[i] = (Packet *)(malloc(sizeof(Packet)));

	CreateTimers();

	struct sigaction signalAction;
	signalAction.sa_sigaction = SignalHandler;
	signalAction.sa_flags = SA_SIGINFO;
	sigemptyset(&signalAction.sa_mask);
	if(sigaction(SIGALRM, &signalAction, NULL) == -1)
		die("ERROR : Couldn't Create Signal Handler\n");

	for(int i=0; i<((FilePkts<=WINDOW_SIZE)?FilePkts:WINDOW_SIZE); i++)
	{	
		UnAcked[i] = ConstructPacket(fp, UnAcked[i]);
		if(UnAcked[i]->RelayNode == 0)
			receiverAddr.sin_port = htons(RELAY0_PORT);
		else
			receiverAddr.sin_port = htons(RELAY1_PORT);
		fprintf(ClientLogs, "Sending Packet...\n");
		if(sendto(clientSocket, UnAcked[i], sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &receiverAddr, AddrLen) == -1)
			die("ERROR : Sending Packets to Relay Node\n");
		if(timer_settime(*timer[i], 0, &timerValue[i], NULL) == -1)
			die("ERROR : Resetting Timer...\n");
		printf(BLUE);
		if(UnAcked[i]->RelayNode==0)
			PrintLog("S", UnAcked[i], "CLIENT", "RELAY1");
		else
			PrintLog("S", UnAcked[i], "CLIENT", "RELAY2");
		printf(RESET);		
		PrintPacket(UnAcked[i]);
	}
	int AckIndex;
	bool AckedPackets[FilePkts], AllPacketsAcked = 0;
	memset(&AckedPackets, 0, FilePkts);
	while(1)
	{
		recvStat = recvfrom(clientSocket, RecvPacket, sizeof(Packet), MSG_WAITALL, (struct sockaddr *) &receiverAddr, &AddrLen); 
		if(recvStat == 0)
			die("ERROR : Receiving Packet from Relay\n");
		if(recvStat == -1)
			continue;
		AckedPackets[RecvPacket->SeqNo] = 1;
		for(AckIndex=0; AckIndex<FilePkts; AckIndex++)
			if(AckedPackets[AckIndex]==0)
				break;
		if(AckIndex==FilePkts)
			AllPacketsAcked = 1;
		else
			AllPacketsAcked = 0;
		printf(GREEN);
		if(RecvPacket->RelayNode==0)
			PrintLog("R", RecvPacket, "RELAY1", "CLIENT");
		else
			PrintLog("R", RecvPacket, "RELAY2", "CLIENT");
		printf(RESET);		
		fprintf(ClientLogs, "Recieved Packet...\n");
		PrintPacket(RecvPacket);
		if(AllPacketsAcked==1)
			die("File Sent to Server Successfully\n");	
				
		if(Sequence < FilePkts)
		{
			for(int i=0 ; i<WINDOW_SIZE; i++)
			{
				if(UnAcked[i]!= NULL && UnAcked[i]->SeqNo == RecvPacket->SeqNo)
				{
					UnAcked[i] = ConstructPacket(fp, UnAcked[i]);
					if(UnAcked[i]->RelayNode == 0)
						receiverAddr.sin_port = htons(RELAY0_PORT);
					else
						receiverAddr.sin_port = htons(RELAY1_PORT);
					fprintf(ClientLogs, "Sending Packet...\n");
					if(sendto(clientSocket, UnAcked[i], sizeof(Packet), MSG_CONFIRM, (struct sockaddr *) &receiverAddr, AddrLen) == -1)
						die("ERROR : Sending Packets to Relay Node\n");
					if(timer_settime(*timer[i], 0, &timerValue[i], NULL) == -1)
						die("ERROR : Resetting Timer\n");
					printf(BLUE);
					if(UnAcked[i]->RelayNode==0)
						PrintLog("S", UnAcked[i], "CLIENT", "RELAY1");
					else
						PrintLog("S", UnAcked[i], "CLIENT", "RELAY2");
					printf(RESET);					
					PrintPacket(UnAcked[i]);
				}
			}
		}
	}
	close(clientSocket);
	fclose(ClientLogs);
	fclose(fp);
	return 0;
}
