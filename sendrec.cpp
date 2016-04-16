#include "sendrec.h"

int bsocket, lsocket;

void die(char * s)
{
    perror(s);
    exit(1);
}

void udp_init(int localPort){
    struct sockaddr_in laddr, baddr;
    int broadcastEnable=1;
    // broadcast
    if ((bsocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        die("bsocket");
    }
    if (setsockopt(bsocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1)
    {
        die("setsockopt");
    }
    memset((char *) &baddr, 0, sizeof(baddr));
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(BROADCASTPORT);
    baddr.sin_addr.s_addr = INADDR_BROADCAST;

    if( bind(bsocket, (struct sockaddr*)&baddr, sizeof(baddr)) == -1)
    {
        die("bbind");
    }
    // local
    if ((lsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("lsocket");
    }
    memset((char *) &laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(localPort);
    laddr.sin_addr.s_addr = INADDR_ANY;
    //inet_pton(AF_INET, INADDR_ANY, &laddr.sin_addr);

    if( bind(lsocket, (struct sockaddr*)&laddr, sizeof(laddr) ) == -1)
    {
        die("lbind");
    }
}
int udp_Broadcaster(std::string message){
    struct sockaddr_in baddr;
    char sbuff[BUFLEN];
    memset(&sbuff[0], 0, sizeof(sbuff));
    strcat(sbuff, message.c_str());
    memset((char *) &baddr, 0, sizeof(baddr));
    baddr.sin_family = AF_INET;
    baddr.sin_port = htons(BROADCASTPORT);
    baddr.sin_addr.s_addr = INADDR_BROADCAST;

    if (sendto(bsocket, sbuff, BUFLEN, 0, (struct sockaddr*) &baddr, sizeof(baddr)) == -1)
    {
        die("bcast");
    }
        //printf("Sendt packet to %s:%d\n", inet_ntoa(baddr.sin_addr), ntohs(baddr.sin_port));
        printf("Data from sender: %s\n" , sbuff);

    return 0;

}
    





int udp_Sender(std::string message, int localPort, char * ip)
{
    struct sockaddr_in addr;
    char sbuff[BUFLEN];
    memset(&sbuff[0], 0, sizeof(sbuff));
    strcat(sbuff, message.c_str());

    memset((char *) &addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(localPort);
    addr.sin_addr.s_addr = inet_addr(ip);
    //inet_pton(AF_INET, ip, &addr.sin_addr);
    
    if (sendto(lsocket, sbuff, BUFLEN, 0, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        die("sendto()");
        
    }
        //printf("Sendt packet to %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        printf("Data from sender: %s\n" , sbuff);    
 
    return 0;
}

struct code_message udp_Reciever()
{
    struct sockaddr_in addr;
    int recv_len;
    socklen_t slen = sizeof(addr); 
    char rbuff[BUFLEN];
    struct code_message code;
    
    // zero out the structure
    memset((char *) &addr, 0, sizeof(addr));
    // memset((char *) &si_other, 0, sizeof(si_other));
    
    /*si_other.sin_family = AF_INET;
    si_other.sin_port = PORT;
    inet_pton(AF_INET, SADDRESS, &si_other.sin_addr);/
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    inet_pton(AF_INET, OURADDRESS, &si_me.sin_addr);*/
     
    //keep listening for data
    //printf("Waiting for data...\n");
      
    memset(&rbuff[0], 0, sizeof(rbuff)); 
    //try to receive some data, this is a blocking call
    if ((recv_len = recvfrom(lsocket, rbuff, BUFLEN, 0, (struct sockaddr *)&addr, &slen)) == -1)
    {
        die("recvfrom()");
    }
    code.data = (char *) malloc(sizeof(char)*recv_len);
    memcpy(code.data,rbuff, strlen(rbuff));
    code.rip = inet_ntoa(addr.sin_addr);
    code.port = addr.sin_port;

    //print details of the client/peer and the data received
    //printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Data received: %s \n" , rbuff);
    //printf("Code data: %s\n", code.data);
 
    return code;
}

struct code_message udp_recieve_broadcast(){
    int recv_len;
    struct code_message code;
    struct sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    memset((char *) &addr, 0, sizeof(addr));
    char rbuff [BUFLEN];
    memset(&rbuff[0], 0, sizeof(rbuff));
    if((recv_len = recvfrom(bsocket, rbuff, BUFLEN, 0, (struct sockaddr *) &addr, &slen)) == -1)
    {
        die("recvfrombcast");
    }

    code.data = (char *) malloc(sizeof(char)*recv_len);
    memcpy(code.data, rbuff, sizeof(rbuff));
    code.rip= inet_ntoa(addr.sin_addr);
    code.port = addr.sin_port;

    //printf("Received packet from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    printf("Data received: %s\n" , rbuff);
    //printf("Code data: %s\n", code.data);
    return code; 
}

int Bytes(){
    char buff[BUFLEN];
    return recv(bsocket, buff, BUFLEN, 0);
}




void udp_close(){
    close(bsocket);
    close(lsocket);
}
