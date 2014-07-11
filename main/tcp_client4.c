/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, long * len);                       //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calculate the time interval between out and in

int main(int argc, char **argv)
{
	int sockfd,ret;		//sockfd : for purpose of creating a socket  //a way to speak to other programs using standard Unix file descriptors.
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;
	

	if (argc != 2) {
		printf("parameters not match");
	}


	sh = gethostbyname(argv[1]);	
	//get host's information
	//gethostbyname() to do DNS lookups.
	//IPV4 : getaddrinfo() - alternative
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

	printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	addrs = (struct in_addr **)sh->h_addr_list;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);                           //create the socket
	//int socket(int domain, int type, int protocol);
	//AF_INET refers to addresses from the internet, IP addresses specifically. PF_INET refers to anything in the protocol, usually sockets/ports.
	//socket() simply returns to you a socket descriptor that you can use in later system calls, or -1 on
	//error.
	
	
	if (sockfd <0)
	{
		printf("error in socket");
		exit(1);
	}
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYTCP_PORT); // -class of functions can help keep things portable by transforming the numbers into  Network Byte Order
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr)); //void * memcpy ( void * destination, const void * source, size_t num );
	//Copies the values of num bytes from the location pointed by source directly to the memory block pointed by destination.
	bzero(&(ser_addr.sin_zero), 8);
	ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));         //connect the socket with the host
	//If you are connect()ing to a remote machine and you don't care what your local port is (as is the case
	//with telnet where you only care about the remote port), you can simply call connect(), it'll check to
	//see if the socket is unbound, and will bind() it to an unused local port if necessary.
	if (ret != 0) {
		printf ("connection failed\n"); 
		close(sockfd); 
		exit(1);
	}
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}

	ti = str_cli(fp, sockfd,&len);                       //perform the transmission and receiving
	rt = (len/(float)ti);                                         //caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	fclose(fp);
//}
	exit(0);
}

float str_cli(FILE *fp, int sockfd,  long * len)
{
	char *buf;
	long lsize, ci;
	char sends[DATALEN];
	struct ack_so ack;
	int n, slen;
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;

	fseek (fp , 0 , SEEK_END);  //The fseek function sets the file position indicator for the stream pointed to by stream. 
	//The new position, measured in bytes, is obtained by adding offset bytes to the position specified by whence.
	lsize = ftell (fp); //The ftell function obtains the current value of the file position indicator for the stream pointed to by stream.
	rewind (fp); //The rewind function sets the file position indicator for the stream pointed to by stream to the beginning of the file
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	gettimeofday(&sendt, NULL);			//get the current time
	uint8_t prev = 0;				
	while(ci<= lsize)
	{
		if ((lsize+1-ci) <= DATALEN)
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
		memcpy(sends, (buf+ci), slen);
		n = send(sockfd, &sends, slen, 0); //int send(int sockfd, const void *msg, int len, int flags);
		//send() returns the number of bytes actually sent out
		if(n == -1) {
			printf("send error!");								//send the data
			exit(1);
		}
		ci += slen;
		
		if ((n= recv(sockfd, &ack, 2, 0))==-1)                                     //receive the ack  //int recv(int sockfd, void *buf, int len, int flags);
		{
			printf("error when receiving\n");
			exit(1);
		}

		if( ack.num != prev && ack.len == 0)
		{
			if(prev==1)
			prev=0;
			else if(prev==0)
			prev=1;
		}
		else
		printf("error in transmission\n");
		
	}
			
	gettimeofday(&recvt, NULL);
	*len= ci;                                                         //get current time
	tv_sub(&recvt, &sendt);                                                                 // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
