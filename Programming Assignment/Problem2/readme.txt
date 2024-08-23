The following instructions are to be followed to execute the Selective Repeat Protocol used for sending files.

1) Ensure that the input file named "input.txt" is present in your working directory.

2) Open 4 seperate terminals

3) In the first terminal, type the following commands to run the file server.c
	$> gcc server.c -o server
	$> ./server

4) In the second terminal, type the following commands to run the file Relay1.c
	$> gcc Relay1.c -o Relay1
	$> ./Relay1

5) In the third terminal, type the following commands to run the file Relay2.c
	$> gcc Relay2.c -o Relay2
	$> ./Relay2

6) In the fourth terminal, type the following commands to run the file client.c. The -lrt and -lm arguments are for linking in timer and mathematical functionalities respectively.
	$> gcc client.c -o client.c -lrt -lm
	$> ./client

In case either of the relays or both of them aren't running, the client will still send packets to the relay, but since the acknowledgment is not received in the client side, it will keep on sending packets until both the relays are connected.


7) The output is observed on the terminals individally for each file in the specified format immediately as Packets start getting transferred. The combined log file is stored in the file Logs.txt
	Blue Colour indicates a packet being sent from a Node
	Green Colour indicates a packet being receivedto a Node
	Red Colour in the terminal indicates a packet being dropped at a relay node or a timeout occuring at client
	Yellow Colour indicates that a packet is being resent from client.


8) After the whole file is sent, the following files are created
output.txt -> Contains the output file
ClientLogs.txt -> Contains the log from the client side.
ServerLogs.txt -> Contains the log from the server side.
Relay1Logs.txt -> Contains the log from the Relay Node 1 side
Relay2Logs.txt -> Contains the log from the Relay Node 2 side
Logs.txt -> Contains the combined log from all the Nodes sorted by Timestamp

Each log in individual log files has the format

Received Packet... / Sending Packet... / Resending Packet... / Dropping Packet ...
*******************************
PAYLOAD : 		// Message of the packet
SIZE : 			// Size of the Message
SEQUENCE NUMBER : 	// Sequence number in number of bytes read till now
LAST PACKET : 		// Check whether the present packet is the last packet or not
DATA/ACK: 		// Whether it is a Data packet or an Acknowledgement packet
RELAY NODE : 		// Relay Node through the packet is being travel
*******************************

Logs.txt has logs of the format as mentioned.

NODE NAME	EVENT		TIMESTAMP	DATA/ACK	SEQUENCE NO	SOURCE		DESTINATION


Some important #defines are WINDOW_SIZE indicating number of packets per window and DELAY_TIME indicating the maximum delay for processing packet at relay nodes in milliseconds.


How has the timer been implemented?
I have made an array of timers of WINDOW_SIZE and a signalling event for each of the timers. So, whenever the retransmission time is crossed in the timers, it sends a signal to the Signal handler to retransmit the packets to the relay node which are stored temporarily in the UnAckedPackets array. A delay has been added to the packets data packets entering the Relay nodes before heading off to the server


How has the transmission of packtes over two simultaneous connections taken place?
The basic idea is that the even numbered packets are sent in relay node 1 and odd numbered packets are sent in relay node 2 until the WINDOW_SIZE is filled. The moment CLIENT receives an ack, I send the next packet to the corresponding relay node. In case of a timeout, I resend the same packet from client to the same relay node and continue the process. Retransmission of packets takes place untill the packet finally reaches. One thing important to note is that in this working, the odd and even numbered packets travel via predefined routes but their acknowledgements can come to any of the relay nodes and back to the client and need not necessarily travel the same route as the packet. 























































