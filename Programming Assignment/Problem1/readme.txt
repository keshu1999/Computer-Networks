The following instructions are to be followed to execute the Multi-Channel Stop-and-Wait Protocol used for sending files. 

1) Ensure that the input file named "input.txt" is present in your working directory.

2) Open 2 seperate terminals

3) In the first terminal, type the following commands to run the file server.c
	$> gcc server.c -o server
	$> ./server

4) In the second terminal, type the following commands to run the file client.c. The -lrt and -lm arguments are for linking in timer and mathematical functionalities respectively.
	$> gcc client.c -o client -lrt -lm
	$> ./client

5) The output is observed on the terminals in the specified format immediately as Packets start getting transferred. 
	Blue Colour indicates a packet being sent over a Channel
	Green Colour indicates a packet being received over a channel
	Red Colour in the server terminal indicates a packet being dropped
	For the packets being resent, "Resending Packets" is displayed with Yellow colour

6) After the whole file is sent, the following files are created
output.txt -> Contains the output file
clientLogs.txt -> Contains the log from the client side.
serverLogs.txt -> Contains the log from the server side.

Each log has the format

Received... / Sending... / Resending Packet... / Dropping Packet ...
*******************************
PAYLOAD : 		// Message of the packet
SIZE : 			// Size of the Message
SEQUENCE NUMBER : 	// Sequence number in number of bytes read till now
LAST PACKET : 		// Check whether the present packet is the last packet or not
DATA/ACK: 		// Whether it is a Data packet or an Acknowledgement packet
CHANNEL NUMBER : 	// Channel through which the packet is sent
*******************************


How has the timer been implemented?
I have made an array of 2 timers and a signalling event for each of the timers. So, whenever the retransmission time is crossed in the timers, it sends a signal to the Signal handler to retransmit the packets to the same channel which are stored temporarily in the UnAckedPackets array. For checking whether any of the sockets have received a packet, I have used polling with a POLLIN event. The moment any acknowledgement/packet comes over any channel, I probe through both the channels to discover where the notification has been received. In case of an acknowledgement being received at the client side, the timer gets resetfor the next packet.


How has the transmission of packtes over two simultaneous connections taken place?
The basic idea is that first two packets are sent over the 2 channels. The moment one of them receives an ack, I send the next packet to the server. In case of a timeout, I resend the same packet and continue the process. If one of the channel is continuously dropping packets (at the server), then the subsequent packets are sent in the other channel. Retransmission of packets over the former channel takes place until the acknowledgement is received.   
 

