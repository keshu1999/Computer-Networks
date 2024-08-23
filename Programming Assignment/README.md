# File Transfer Protocols: Multi-Channel Stop-and-Wait & Selective Repeat over UDP

## Overview

This project implements file transfer mechanisms utilizing the **Multi-Channel Stop-and-Wait Protocol** and the **Selective Repeat Protocol** over **User Datagram Protocol (UDP)**. Both protocols are designed to ensure reliable data transfer in the presence of packet loss, delay, and network unreliability, typically found in real-time applications.

## Methodology

### Client-Side Operations

1. **Packet Generation**: The client constructs data packets for each element within the sliding window.
2. **Transmission Strategy**:
   - Distributes packets across multiple relays: odd-numbered packets are transmitted to `Relay2`, while even-numbered packets are sent to `Relay1`.
   - Uses the `select()` system call to await acknowledgment (ACK) packets:
     - `>0`: Indicates receipt of one or more ACKs, marking the corresponding packets as received.
     - `=0`: Indicates a timeout event, triggering retransmission of all unacknowledged packets.

### Relay-Side Operations

1. **Packet Handling**:
   - Receives packets from the client.
   - Introduces artificial delay using a random floating-point value (0-2 seconds) to simulate network latency.
   - Employs a **Packet Drop Rate (PDR)** to probabilistically drop packets (omit sending ACK).
   - Uses a timed receive call to obtain responses from the server, acknowledging packets back to the client.

### Server-Side Operations

1. **Data Reception**:
   - Accepts packets from the relay.
   - Checks sequence numbers for expected order:
     - **Out-of-order packets**: Buffered if queue space permits; otherwise, discarded.
     - **In-order packets**: Written directly to the destination file.
   - If contiguous buffered packets are available, they are sequentially written to maintain order.
   - Sends ACKs for successfully processed packets to the corresponding relay.

## Instructions to Run

The instructions to run each protocol are present in the individual folders `Problem1` and `Problem2`

### Generated Output

- **destination_file.txt**: Final file created by the server.
- **Logs**:
  - `client.log` - Client activity log.
  - `server.log` - Server activity log.
  - `relay1.log` - `Relay1` activity log.
  - `relay2.log` - `Relay2` activity log.

### Log Analysis

To collate and sort logs by timestamp, use the command:
```bash
sort *.log >> combinedLogs.log
```
This generates a comprehensive, time-ordered log in `combinedLogs.log`.