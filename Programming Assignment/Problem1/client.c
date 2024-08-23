#include "packet.h"

void die(char *message)
{
	perror(message);
	exit(1);
}


void PrintPacket(Packet* pack)
{
	fprintf(clientLogs,"*******************************\n");
	fprintf(clientLogs,"PAYLOAD : 		%s\n", pack->payload);
	fprintf(clientLogs,"SIZE : 			%d\n", pack->payloadsize);
	fprintf(clientLogs,"SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(clientLogs,"LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(clientLogs,"DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(clientLogs,"CHANNEL NUMBER : 	%d\n", pack->channel_info);
	fprintf(clientLogs,"*******************************\n\n\n");
	return;
}


void ClientPacketTrace(Packet *pack)
{
	printf(BLUE"\n*******************************\n");
	printf("SENT PKT : Seq. No = %d of Size %d Bytes\n\t\tfrom Channel %d\n", pack->SeqNo, pack->payloadsize, pack->channel_info+1);
	if(pack->lastPacket==1)
		printf("Last Packet\n");
	printf("*******************************\n\n"RESET);
}


void ClientACKTrace(Packet *pack)
{
	printf(GREEN"\n*******************************\n");
	printf("RCVD ACK : For PKT with Seq. No = %d\n\t\tfrom Channel %d\n", pack->SeqNo, pack->channel_info+1);
	if(pack->lastPacket==1)
		printf("Last Acknowledgement\n");
	printf("*******************************\n\n"RESET);
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


static void SignalHandler(int signal, siginfo_t *siginfo, void *uc)
{
	timer_t *temptime;
	temptime = siginfo->si_value.sival_ptr;
	if(*temptime == *timer[0])
	{
        	printf(YELLOW"Resending Packet...");
        	if(send(socket1, UnAckedPacks[0], sizeof(Packet), 0) == -1)
			die("ERROR : Couldn't Resend through Channel 1\n");
		if(timer_settime(*timer[0], 0, &timerVal[0], NULL) == -1)
            		die("ERROR : Resetting Timer");
		fprintf(clientLogs, "Resending Packet...\n");
        	PrintPacket(UnAckedPacks[0]);
		ClientPacketTrace(UnAckedPacks[0]);
		
    	}
    	else if(*temptime == *timer[1])
    	{
		printf(YELLOW"Resending Packet...");
        	if(send(socket2, UnAckedPacks[1], sizeof(Packet), 0) == -1)
            		die("ERROR : Couldn't Resend through Channel 1\n");
       		if(timer_settime(*timer[1], 0, &timerVal[1], NULL) == -1)
            		die("ERROR : Restting Timer");
		fprintf(clientLogs, "Resending Packet...\n");  
        	PrintPacket(UnAckedPacks[1]);
		ClientPacketTrace(UnAckedPacks[1]);
    	}
    	else
		die("ERROR : Setting TIMER\n");
	return;
}


void CreateTimers()
{
	timer[0] = (timer_t*)malloc(sizeof(timer_t));
	timer[1] = (timer_t*)malloc(sizeof(timer_t));

	struct sigevent event[2];
	event[0].sigev_notify = SIGEV_SIGNAL;
	event[0].sigev_signo = SIGALRM;
	event[0].sigev_value.sival_ptr = timer[0];
	timer_create(CLOCK_REALTIME, &event[0], timer[0]);
	timerVal[0].it_interval.tv_sec = 0;
	timerVal[0].it_interval.tv_nsec = 0;
	timerVal[0].it_value.tv_sec = RETRANSMIT_TIME;
	timerVal[0].it_value.tv_nsec = 0;

	event[1].sigev_notify = SIGEV_SIGNAL;
	event[1].sigev_signo = SIGALRM;
	event[1].sigev_value.sival_ptr = timer[1];
	timer_create(CLOCK_REALTIME, &event[1], timer[1]);
	timerVal[1].it_interval.tv_sec = 0;
	timerVal[1].it_interval.tv_nsec = 0;
	timerVal[1].it_value.tv_sec = RETRANSMIT_TIME;
	timerVal[1].it_value.tv_nsec = 0;
	return;	
}


Packet* ConstructPacket(FILE* fp, bool channel)
{
	Packet* pack = (Packet *)malloc(sizeof(Packet));
	int bytesRead = fread(pack->payload, 1, PACKET_SIZE, fp);
	if(bytesRead == PACKET_SIZE)
	{
		pack->payloadsize = PACKET_SIZE;
        	pack->SeqNo = Sequence*PACKET_SIZE;
        	pack->lastPacket = 0;
        	pack->DATA_ACK = 0;
        	pack->channel_info = channel;
        	pack->numPackets = FilePkts;
	}
	else
	{
		if(!feof(fp))
			die("ERROR : Reading from file input.txt\n");	
        	else
        	{
			pack->payloadsize = bytesRead;
			pack->SeqNo = Sequence*PACKET_SIZE;
			pack->lastPacket = 1;
			pack->DATA_ACK = 0;
			pack->channel_info = channel;
			pack->numPackets = FilePkts;
        	}
    	}
	Sequence++;
    	return pack;
}



int main()
{
	socket1 = socket(AF_INET, SOCK_STREAM, 0);
	if (socket1 == -1)
		die("ERROR : Couldn't Connect to 1st Socket\n");

	socket2 = socket(AF_INET, SOCK_STREAM, 0);
	if (socket2 == -1)
		die("ERROR : Couldn't Connect to 2nd Socket\n");

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET; 
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	serverAddr.sin_port = htons(PORT); 
 
	if(connect(socket1, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) 
		die("ERROR : Connecting Socket 1 With Server\n");
	if(connect(socket2, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) 
		die("ERROR : Connecting Socket 1 With Server\n");
	
	clientLogs = fopen("clientLogs.txt", "w");
	if(clientLogs == NULL)
		die("ERROR : Opening file logs\n");

	FILE* fp = fopen("input.txt", "r");
	if(fp == NULL)
		die("ERROR : Opening file input.txt\n");

	Sequence = 0;	
	int filesize = NoofChars("input.txt");
	FilePkts = ceil((double)(filesize)/(PACKET_SIZE));
	bool AckRecved[FilePkts], last;
	memset(AckRecved, 0, FilePkts);

	CreateTimers();

	struct sigaction signalAction;
	signalAction.sa_sigaction = SignalHandler;
	signalAction.sa_flags = SA_SIGINFO;
	sigemptyset(&signalAction.sa_mask);
	if(sigaction(SIGALRM, &signalAction, NULL) == -1)
		die("ERROR : Couldn't Create Signal Handler\n");
    
	if(FilePkts == 1)
	{   
		UnAckedPacks[0] = ConstructPacket(fp, 0);
		if(send(socket1, UnAckedPacks[0], sizeof(Packet), 0) == -1)
            		die("ERROR : Couldn't Send a single Packet\n");
        	if(timer_settime(*timer[0], 0, &timerVal[0], NULL) == -1)
            		die("ERROR : Resetting Timer\n");
		fprintf(clientLogs, "Sending...\n");
        	PrintPacket(UnAckedPacks[0]);
		ClientPacketTrace(UnAckedPacks[0]);       
	}
   
	else if(FilePkts >= 2)
	{
		UnAckedPacks[0] = ConstructPacket(fp, 0);
		UnAckedPacks[1] = ConstructPacket(fp, 1);
        	
		if(send(socket1, UnAckedPacks[0], sizeof(Packet), 0) == -1 || send(socket2, UnAckedPacks[1], sizeof(Packet), 0) == -1)
			die("ERROR : Couldn't Send First Two Packets to Server");

        	if(timer_settime(*timer[0], 0, &timerVal[0], NULL) == -1 || timer_settime(*timer[1], 0, &timerVal[1], NULL) == -1)
            		die("ERROR : Resetting Timer\n");
		fprintf(clientLogs, "Sending...\n");				
		PrintPacket(UnAckedPacks[0]);	
		ClientPacketTrace(UnAckedPacks[0]);
		fprintf(clientLogs, "Sending...\n");		
		PrintPacket(UnAckedPacks[1]);
		ClientPacketTrace(UnAckedPacks[1]);
	}

	struct pollfd SocketPoll[2];
	SocketPoll[0].fd = socket1;
	SocketPoll[0].events = POLLIN;
	SocketPoll[1].fd = socket2;
	SocketPoll[1].events = POLLIN;

	Packet* RecvPacket = (Packet*)malloc(sizeof(Packet));

	int clientPoll, clientRecv, packDeliv = 0;
		
    	while(packDeliv < FilePkts)
    	{
		clientPoll = poll(SocketPoll, 2, -1);
		if ((clientPoll < 0))
            		continue;

		for(int i=0; i<2; i++)
		{
			if(SocketPoll[i].revents == POLLIN)   
			{
				clientRecv = recv(SocketPoll[i].fd, RecvPacket, sizeof(Packet), 0);
				if(clientRecv == -1)
					continue;
				else if(clientRecv == 0)
					die("Connection Closed\n");

				else if(RecvPacket->DATA_ACK == 1)
				{
					packDeliv = -1;
					int packetIndex = RecvPacket->SeqNo / PACKET_SIZE;
					if(AckRecved[packetIndex] == 1)
						continue;

					AckRecved[packetIndex] = 1;
					for(int j = 0 ; j<FilePkts ; j++)
					{
						if(AckRecved[j]==0)
						{
							packDeliv=0;
							break;
                        			}
                    			}
					
					fprintf(clientLogs, "Recieved...\n");
					PrintPacket(RecvPacket);
					ClientACKTrace(RecvPacket);
					if(packDeliv == FilePkts)
					{
						close(SocketPoll[0].fd);
						close(SocketPoll[1].fd);
						printf("Exiting...\n");
						exit(0);
					}
					else if(RecvPacket->lastPacket == 0)
					{
						UnAckedPacks[i] = ConstructPacket(fp, i);
						if(send(SocketPoll[i].fd, UnAckedPacks[i], sizeof(Packet), 0) == -1)
							die("ERROR Sending Packets to Server\n");

						if(timer_settime(*timer[i], 0, &timerVal[i], NULL) == -1)
							die("ERROR : Resetting Timer\n");

						fprintf(clientLogs,"Sending...\n");
						PrintPacket(UnAckedPacks[i]);
						ClientPacketTrace(UnAckedPacks[i]);
					}
					else if(RecvPacket->lastPacket == 1)
					{
						timerVal[i].it_interval.tv_sec = 0;
						timerVal[i].it_interval.tv_nsec = 0;
						timerVal[i].it_value.tv_sec = 0;
						timerVal[i].it_value.tv_nsec = 0;
						if(timer_settime(*timer[i], 0, &timerVal[i], NULL) == -1)
							die("ERROR : Resetting Timer\n");
					}
				}
			}
		}
	}
}
