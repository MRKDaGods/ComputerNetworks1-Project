packet Packet {
    int frameType;  // 0: NACK, 1: ACK, 2: Data
    int seqNum;
    string payload;
    int parity;
    int ackNum;     // ACK/NACK number
}
