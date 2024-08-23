#include "packet.h"

void die(char *message)
{
	perror(message);
	exit(1);
}


bool DropOrSend()
{
	if(rand()%100 < DROP_RATE)
		return 0;
	return 1;
}


void PrintPacket(Packet* pack)
{
	fprintf(serverLogs, "*******************************\n");
	fprintf(serverLogs, "PAYLOAD : 		%s\n", pack->payload);
	fprintf(serverLogs, "SIZE : 			%d\n", pack->payloadsize);
	fprintf(serverLogs, "SEQUENCE NUMBER : 	%d\n", pack->SeqNo);
	fprintf(serverLogs, "LAST PACKET : 		%s\n", pack->lastPacket==0?"NO":"YES");
	fprintf(serverLogs, "DATA/ACK: 		%s\n", pack->DATA_ACK==0?"DATA":"ACK");
	fprintf(serverLogs, "CHANNEL NUMBER : 	%d\n", pack->channel_info);
	fprintf(serverLogs, "*******************************\n\n\n");
	return;
}


void ServerPacketTrace(Packet *pack)
{
	printf(GREEN"\n*******************************\n");
	printf("RCVD PKT : Seq. No = %d of Size %d Bytes\n\t\tfrom Channel %d\n", pack->SeqNo, pack->payloadsize, pack->channel_info+1);
	if(pack->lastPacket==1)
		printf("Last Packet\n");
	printf("*******************************\n\n"RESET);
}


void ServerACKTrace(Packet *pack)
{
	printf(BLUE"\n*******************************\n");
	printf("SENT ACK : For PKT with Seq. No = %d\n\t\tfrom Channel %d\n", pack->SeqNo, pack->channel_info+1);
	if(pack->lastPacket==1)
		printf("Last Acknowledgement\n");
	printf("*******************************\n\n"RESET);
}


Packet* ConstructAckPacket(Packet* receivedPack)
{
	Packet* ackpack = malloc(sizeof(Packet));
	sprintf(ackpack->payload, "Acknowledgement for Packet Number %d", receivedPack->SeqNo/PACKET_SIZE + 1);	
	ackpack->payloadsize = sizeof(ackpack->payload);
        ackpack->SeqNo = receivedPack->SeqNo;
        ackpack->lastPacket = receivedPack->lastPacket;
        ackpack->DATA_ACK = 1;
        ackpack->channel_info = receivedPack->channel_info;
    	return ackpack;
}


int main()
{
	srand(time(0));
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
   	if(serverSocket == -1)
   		die("ERROR : Making Server Socket\n");

	int socketoptions = 1;
   	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &socketoptions, sizeof(socketoptions)) == -1) 
    		die("ERROR : Setting Socket Options\n");

	struct sockaddr_in serverAddr;
	int serverLen=sizeof(serverAddr);
    
	serverAddr.sin_family = AF_INET; 
    	serverAddr.sin_addr.s_addr = INADDR_ANY; 
   	serverAddr.sin_port = htons(PORT); 

    	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
		die("ERROR : Binding Server\n");

	if (listen(serverSocket, 2) < 0) 
    		die("ERROR : Couldn't listen in Server\n");
	printf("Now Listening\n");
	printf("Connect the Clients\n");
	
    	struct pollfd Polls[2];
	Polls[0].events = POLLIN;
	Polls[1].events = POLLIN;

	Packet *ReceivedPkt, *AckPkt;
	ReceivedPkt = (Packet*)malloc(sizeof(Packet));
	
	int MainPoll, recvcond, pktcount;

	serverLogs = fopen("serverLogs.txt", "w");
	if(serverLogs == NULL)
		die("ERROR : Opening file logs\n");

	FILE* fp = fopen("output.txt", "w+");
	if(fp==NULL)
		die("ERROR : Opening file output.txt\n");

	int fpfd = fileno(fp);
	while(1)
	{
		Polls[0].fd = accept(serverSocket, (struct sockaddr *)&serverAddr, (socklen_t*)&serverLen);
    		if(Polls[0].fd < 0)
    			die("ERROR : Couldn't Accept Client 1\n");
		
		Polls[1].fd = accept(serverSocket, (struct sockaddr *)&serverAddr, (socklen_t*)&serverLen);
    		if(Polls[1].fd < 0)
    			die("ERROR : Couldn't Accept Client 2\n");

    		pktcount = 1;
	    	while(pktcount != -1)
	    	{
	    		MainPoll = poll(Polls, 2, -1);				// Infintie Time
	    		if ((MainPoll < 0))
		    		die("ERROR : Couldn't Poll\n");
		   
			for(int i=0; i<2; i++)
			{
				if(Polls[i].revents == POLLIN)
				{
					recvcond = recv(Polls[i].fd, ReceivedPkt, sizeof(Packet), 0);
					if(recvcond == -1)
						die("ERROR : Receiving Packet\n");
					else if(recvcond == 0)					//connection closed
					{
						printf("Connection Closed\n");
						pktcount = -1;
					}
					else
					{
						if(DropOrSend() || ReceivedPkt->lastPacket==1)			//Packet not dropped
						{
							fprintf(serverLogs, "Received...\n");
							PrintPacket(ReceivedPkt);
							ServerPacketTrace(ReceivedPkt);
							pwrite(fpfd, ReceivedPkt->payload, ReceivedPkt->payloadsize, ReceivedPkt->SeqNo);		                    
							AckPkt = ConstructAckPacket(ReceivedPkt);
							
							fprintf(serverLogs, "Sending...\n");
							if(send(Polls[i].fd, AckPkt, sizeof(Packet), 0) == -1)
				    				die("ERROR : Couldn't Send Acknowledgement Packet\n");
				    			PrintPacket(AckPkt);
							ServerACKTrace(AckPkt);
				    			
				    			if(pktcount < ReceivedPkt->numPackets)
				    				pktcount++;
				    			else
				    				pktcount = -1;
				    		}
				    		else								//Packet dropped
				    		{
				    			fprintf(serverLogs, "Dropping Packet...\n");
				    			PrintPacket(ReceivedPkt);
							printf(RED"\n*******************************\n");
							printf("DROPPED PKT : Seq. No = %d of Size %d Bytes\n\t\tfrom Channel %d\n", ReceivedPkt->SeqNo, ReceivedPkt->payloadsize, ReceivedPkt->channel_info+1);
							if(ReceivedPkt->lastPacket==1)
								printf("Last Packet\n");
							printf("*******************************\n\n"RESET);
				    			continue;
				    		}
			    			
					}  			
				}
			}
	    	}
    		close(Polls[0].fd);
    		close(Polls[1].fd);
	}
	close(serverSocket);
	return 0;
}
