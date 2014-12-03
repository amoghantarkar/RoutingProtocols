#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <stdint.h>


#include "../include/global.h"
#include "../include/logger.h"
#include "../include/server.h"


#define ROWS2 8
#define ROWS3 12
/*Declarations*/
void routing_init();
void DVT();
int setup_udp(const char* pno);
void command(char buff[200]);
void *createpacket();
void sendneighbors(void *packetptr);
void *get_in_addr(struct sockaddr *sa);
void Bellmanford();
void receivepacket();
int* findmin(int array[10]);

int infnum=65535;
int stepflag=0;
int myport;
int id,sockfd;
extern const char* arg2;
extern const char* arg4;
int numser,numneib; // number of server and number of neighbors..

int packetnum=0;
//[PA3] Routing Table Start

int routingtable[6][6];		// the routing table matrix

struct localnode
{
	int status[6];
	time_t starttime[6];
	int count[6];
	int nexthop[6];
	int bool[6];
};
struct localnode ninfo;

/*Used to Save the all the routers(here servers involved) information*/
struct nodedata
{
	int nid;
	char ipaddr[INET_ADDRSTRLEN];
	int port;
	struct localnode ninfo;

};
struct nodedata ndata[6];

//[PA3] Routing Table End

/*This function extracts the topology file content and sets up the initial routing table as acc to Dist Vect Algo specifications*/
void routing_init()
{
	// This will search the last line in the file and extract the id

	int numneib;
	int i,j;
	FILE *fd;                           // File pointer
	char line[256];
	char myip[INET_ADDRSTRLEN];

	size_t len = 0;
	static const long max_len = 55+1;  // define the max length of the line to read

	/*initializing the routing table to infinity (ie -infnum) here*/
	for(i=1;i<6;i++)
	{
		for(j=1;j<6;j++)
		{	if(i==j)
		{
			routingtable[i][j]=0;		//initialized the distance vector from self to self = zero
		}
		else
		{
			routingtable[i][j]=infnum;
		}
		}
	}
	for(i=1;i<6;i++)
	{
		for(j=1;j<6;j++)
		{
			ndata[i].ninfo.count[j]=0;
			ndata[i].ninfo.bool[j]=0;
		}
	}

	if ((fd = fopen(arg2, "r")) != NULL)
	{
		/* read the first line and register the number of servers*/
		fgets(line,100,fd);	//reads till the end of the line
		sscanf(line,"%d",&numser);	//reads the line

		/*	read the second line and save the number of neighbors*/
		fgets(line,100,fd);
		sscanf(line,"%d",&numneib);	//store the number of neighbors

		/*storing the information of all the nodes , its ipaddresses and its corresponding port in struct nodedata ndata[]*/
		int cid,k;
		char cipaddr[13];
		int cport;
		for(k=0;k<numser;k++)
		{
			fgets(line,100,fd);
			sscanf(line,"%d%s%d",&cid,&cipaddr,&cport);
			ndata[cid].nid=cid;
			strcpy(ndata[cid].ipaddr,cipaddr);
			ndata[cid].port=cport;
		}


		int id1,id2,ccost;
		/*reads the input last cost lines from the topology table and stores the values in the routing table matrix*/
		/*initially assumed that there is no neighbor to the server*/
		for(j=1;j<6;j++)
		{	for(k=1;k<6;k++)
			{
			ndata[j].ninfo.status[k]=0;
			ndata[j].ninfo.nexthop[k]=-1;
			}
		}

		while(!feof(fd))
		{
			fgets(line,100,fd);
			sscanf(line,"%d%d%d",&id1,&id2,&ccost);					// notes node id for the cost from id1->to id2
			routingtable[id1][id2]=ccost;		// stores the cost in the routing table matrix
			ndata[id1].ninfo.status[id2]=1;
			ndata[id1].ninfo.nexthop[id2]=id2;

		}

		/*searching the structure with the id to find the ipaddress and port*/
		id=id1;
		strcpy(myip,ndata[id1].ipaddr);
		myport=ndata[id1].port;
		k=0;

		fclose(fd);                               // close the file
	}
	printf("\nMy Server information\nIP:%s\tID:%d\tPORT:%d\n",ndata[id].ipaddr,id,ndata[id].port);
	printf("\n");
}

int setup_udp(const char* pno)
{

	//http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
	int sockfd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL,pno, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);
	return sockfd;

}
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void command(char buff[200])
{
void *packetptr;
char token[50];
char *tok=token;
int i,j;
if(buff!=NULL)
{
	tok=strtok(buff,"\n ");
	if(strcasecmp(tok,"step")==0)
	{
		//			code for step this triggers sending the update packet to  all its neighbors
		stepflag=1;
		packetptr=createpacket();
		sendneighbors(packetptr);
		cse4589_print_and_log("%s:SUCCESS\n",tok);
		stepflag=0;

	}
	else if(strcasecmp(tok,"packets")==0)
	{
		printf("%d",packetnum);
		cse4589_print_and_log("%s:SUCCESS\n",tok);

		packetnum=0;
	}
	else if(strcasecmp(tok,"academic_integrity")==0)
	{
		cse4589_print_and_log("%s:SUCCESS\n",tok);
		cse4589_print_and_log("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity ");

	}
	else if(strcasecmp(tok,"update")==0)
	{


		char token1[50],token2[50],token3[50];
		char *tokn1=token1, *tokn2=token2, *tokn3=token3;
		tokn1=strtok(NULL," "); 	//will store the server-id 1
		tokn2=strtok(NULL," ");		//will store the server id 2
		tokn3=strtok(NULL," ");		// will store the link cost
		int cost;
		if(strcasecmp(tokn3,"inf")==0)
		{
			cost=infnum;
		}
		else
		{
			cost=atoi(tokn3);
		}

		if(atoi(tokn1)==id)
		{
			//to check if the tokn1 is same as the id of the server only then proceed further
			routingtable[atoi(tokn1)][atoi(tokn2)]=cost;
			cse4589_print_and_log("%s:SUCCESS\n",tok);
		}
		else
		{
			printf("The update command <server id 1> should be current server and <server id 2> the other server");
		}
	 }
	else if(strcasecmp(tok,"disable")==0)
	{	char token4[50],token5[50],token6[50];
		char *tokn4=token4, *tokn5=token5, *tokn6=token6;
		tokn4=strtok(NULL," "); 	//will store the server-id 1
		int disableid;
		disableid=atoi(tokn4);
		if(ndata[id].ninfo.status[disableid]==1) //checking if requested is the neighbor only then disable
		{
			routingtable[id][disableid]=infnum;
			ndata[id].ninfo.status[disableid]==-1;
			Bellmanford();
			cse4589_print_and_log("%s:SUCCESS\n",tok);
		}
		else
		{
			printf("Incorrect command usage\n");
			printf("DISABLE USAGE:disable <server­ID>\nDisable  the  link  to  a  given  server.  Doing  this “closes” the connection to a given server  with  server­ID ");
		}
	}

	else if(strcasecmp(tok,"display")==0)
	{
		cse4589_print_and_log("%s:SUCCESS\n",tok);

		printf("Destination id--Nexthop--Cost--\n");
			for(j=1;j<numser+1;j++)
			{	int printid;
				printid=j;
				cse4589_print_and_log("%-15d%-15d%-15d\n",j,ndata[id].ninfo.nexthop[j],routingtable[id][j]);
			}
			/*printf("\nServer's Local routing table with all the distance vectors.\n");

					for(i=1;i<numser+1;i++)
					{
						for(j=1;j<numser+1;j++)
						{
							printf("%-10d|",routingtable[i][j]);
						}
						printf("\n");
					}
*/

	}
	else if(strcasecmp(tok,"crash")==0)
	{
		Bellmanford();

		cse4589_print_and_log("%s:SUCCESS\n",tok);
		fd_set waitfd;
			while(1)
			{
				if(select(1, &waitfd, NULL, NULL, NULL)==-1)
				{
					exit(4);
				}
			}

	}
	else if(strcasecmp(tok,"dump")==0)
	{
			cse4589_print_and_log("%s:SUCCESS\n",tok);
			packetptr=createpacket();
			cse4589_dump_packet(packetptr,(ROWS2+(ROWS3*numser)));
	}
	else
	{	char message1[100]="enter the valid argument";
		char* message=message1;
		cse4589_print_and_log("%s:%s\n",tok,message);

	}

}
}

void *createpacket()
{
//[PA3] Update Packet Start

int k=0;
uint32_t serveripn[numser];
uint16_t serverportn[numser];
uint16_t padding=0;
uint16_t serveridn[numser];
uint16_t costn[numser];
uint16_t serveripportn[numser];
void *packetdata =  malloc (ROWS2+ROWS3*numser);

//[PA3] Update Packet End

uint16_t numberfields=numser;
htons(numberfields);
uint16_t serverport=ndata[id].port;
htons(serverport);

uint16_t lnumberfields;
uint16_t lserverport;
uint32_t lserverip;

struct sockaddr_in ipsa;
char str[INET_ADDRSTRLEN];

memcpy(packetdata,&numberfields,sizeof(numberfields));
packetdata+=sizeof(numberfields);
memcpy(packetdata,&serverport,sizeof(serverport));
packetdata+=sizeof(serverport);

inet_pton(AF_INET, ndata[id].ipaddr, &(ipsa.sin_addr));
uint32_t serverip=ipsa.sin_addr.s_addr;
htonl(serverip);

memcpy(packetdata,&serverip,sizeof(serverip));
packetdata+=sizeof(serverip);


for(k=1;k<numser+1;k++)
{

	inet_pton(AF_INET, ndata[k].ipaddr, &(ipsa.sin_addr));
	serveripn[k]=ipsa.sin_addr.s_addr;
	htonl(serveripn[k]);
	memcpy(packetdata,&serveripn[k],sizeof(serveripn[k]));
	packetdata+=sizeof(serveripn[k]);


	serveripportn[k]=ndata[k].port;
	htons(serveripportn[k]);
	memcpy(packetdata,&serveripportn[k],sizeof(serverportn[k]));
	packetdata+=sizeof(serveripportn[k]);

	htons(padding);
	memcpy(packetdata,&padding,sizeof(padding));
	packetdata+=sizeof(padding);

	serveridn[k]=ndata[k].nid;
	htons(serveridn[k]);
	memcpy(packetdata,&serveridn[k],sizeof(serveridn[k]));
	packetdata+=sizeof(serveridn[k]);

	costn[k]=routingtable[id][k];//check values
	htons(costn[k]);
	memcpy(packetdata,&costn[k],sizeof(costn[k]));
	packetdata+=sizeof(costn[k]);


}
packetdata=packetdata-(ROWS2+(ROWS3*numser));
return packetdata;


}

void sendneighbors(void *packetptr)
{
	//printf("\ninside sendneighbor");
	int k;
	struct sockaddr_in sendaddr;

	char sockp[20];
	char* sockport;
	char* Dest_ip;


	for(k=1;k<6;k++)
	{
			if(k!=id && ndata[id].ninfo.status[k]==1)
			{
				sprintf(sockp,"%d",ndata[k].port);
				sockport=sockp;

				int newfd;  // listen on sock_fd, new connection on new_fd
				struct addrinfo hints, *servinfo, *p;
				struct sockaddr_storage their_addr; // connector's address information
				socklen_t sin_size;
				struct sigaction sa;
				int yes=1;
				char s[INET6_ADDRSTRLEN];
				int rv;

				//http://beej.us/guide/bgnet/output/html/multipage/clientserver.html

				//printf("assigning hints");
				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_DGRAM;
				Dest_ip=ndata[k].ipaddr;

			//	printf("getaddrinfo entering");
				if ((rv = getaddrinfo(Dest_ip,sockport, &hints, &servinfo)) != 0) {
					fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));

				}

				for(p = servinfo; p != NULL; p = p->ai_next)
				{
				if((newfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
				{
					perror("server: socket");
					continue;
				}

				break;
				}

				if (p == NULL)  {
					fprintf(stderr, "server: failed to bind\n");

				}

				sendto(newfd,(char*)packetptr,8+12*numser,0, p->ai_addr,p->ai_addrlen);

			/*	printf("\nsending to");
				printf("newfd: %d\tipaddress %s\t port:%s",newfd,Dest_ip,sockport);
	*/			freeaddrinfo(servinfo);
				close(newfd);

				if(stepflag==0)
				{
				if(ndata[id].ninfo.bool[k]==1)
					{
					ndata[id].ninfo.starttime[k]=time(NULL);
					ndata[id].ninfo.count[k]=ndata[id].ninfo.count[k]+1;
					//even after 3 consequentively sending there is no  update packet from the receiver then it means that neigbor is no more active
						if(ndata[id].ninfo.count[k]>3)
						{
							routingtable[id][k]=infnum;
							//ndata[id].ninfo.status[k]=0;

						}
					}
				}


			}//if

		}//for

}


/*
	receive the packet from the specific file descriptor and then tokenize and store the information in the corresponding distance vector
		Beej's Guide to Network Programming* page 89  and http://www.cs.ucsb.edu/~almeroth/classes/W01.176B/hw2/examples/udp-client.c referenced
 */



void receivepacket()
{

	uint32_t aserveripn[numser];
	uint16_t apadding=0;
	uint16_t aserveridn[numser];
	uint16_t acostn[numser];
	uint16_t aserverportn[numser];

	uint16_t anumberfields;
	uint16_t aserverport;
	uint32_t aserverip;


	int k;
	int j;
	int byte_count;
	struct sockaddr_in addr;
	struct sockaddr_storage cli_addr;
	char rbuff[1000];
	socklen_t fromlen;

	void* recvbuff=malloc(sizeof(rbuff));


	byte_count = recvfrom(sockfd,recvbuff,8+12*numser,0,(struct sockaddr*)&cli_addr,&fromlen);

//	printf("\n received packet %s",recvbuff);

	//beej 9.14. inet_ntop(), inet_pton()

	struct sockaddr_in sa;
	char str[INET_ADDRSTRLEN];

	memcpy(&anumberfields,recvbuff,sizeof(anumberfields));
	ntohs(anumberfields);
	recvbuff+=sizeof(anumberfields);
	memcpy(&aserverport,recvbuff,sizeof(aserverport));
	recvbuff+=sizeof(aserverport);
	ntohs(aserverport);

	memcpy(&aserverip,recvbuff,sizeof(aserverip));
	recvbuff+=sizeof(aserverip);
	ntohl(aserverip);
	sa.sin_addr.s_addr=aserverip;
	inet_ntop(AF_INET, &(sa.sin_addr),str, INET_ADDRSTRLEN);

	uint16_t rcvid; //the distance vector entry.. of the id..this idth line is updated..

	for(k=1;k<6;k++)
	{
		if(strcmp(str,ndata[k].ipaddr)==0)
		{
			rcvid=k;
		}
	}

	cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n", rcvid);

	ndata[id].ninfo.bool[rcvid]=1;
	ndata[id].ninfo.count[rcvid]=0; //received the information so set count to 0
	//find the id from the ipaddr and update the count of the timer


	//again same as the sendpacket function receive the packet and update the corresponding information in the ndata struct and the routingtable


	uint16_t rid;//the column which we have to update..with row = rcvid..
	for(k=1;k<numser+1;k++)
	{
		memcpy(&aserveripn[k],recvbuff,sizeof(aserveripn[k]));
		recvbuff+=sizeof(aserveripn[k]);
		ntohl(aserveripn[k]);
		sa.sin_addr.s_addr=aserveripn[k];
		inet_ntop(AF_INET, &(sa.sin_addr),str, INET_ADDRSTRLEN);


		memcpy(&aserverportn[k],recvbuff,sizeof(aserverportn[k]));
		ntohs(aserverportn[k]);
		recvbuff+=sizeof(aserverportn[k]);

		memcpy(&apadding,recvbuff,sizeof(apadding));
		ntohs(apadding);
		recvbuff+=sizeof(apadding);


		memcpy(&aserveridn[k],recvbuff,sizeof(aserveridn[k]));
		ntohs(aserveridn[k]);
		rid=aserveridn[k];	//find the server's id received to store the routing table..
		recvbuff+=sizeof(aserveridn[k]);

		memcpy(&acostn[k],recvbuff,sizeof(acostn[k]));
		ntohs(acostn[k]);
		routingtable[rcvid][k]=acostn[k];	//store the respective routing table column 's with row rcvid
		recvbuff+=sizeof(acostn[k]);
		cse4589_print_and_log("%-15d%-15d\n",k,acostn[k]);
	}

}


//This function returns the minimum of the array
int* findmin(int array[6])
{
	 int *bellvalues=malloc(sizeof(int)*2);
	 int min=array[0],n,loc;
	for(n=1;n<6;n++)
	{
		if(array[n]<min)
		{
			min=array[n];
			loc=n;
		}
	}
	bellvalues[0]=min;
	bellvalues[1]=loc;
	return bellvalues;
}

void Bellmanford()
{
	int currvector;
	int l;
	int array[6];
	for(l=0;l<6;l++)
	{
		array[l]=9999999;
	}
	int *result;

	int a,b;

// Bellman ford equation..
	for(a=1;a<6;a++)
	{	if(id!=a)
		{
			for(b=1;b<6;b++)
			{
				if(ndata[id].ninfo.status[b]==1)
				{
				array[b]=routingtable[id][b]+routingtable[b][a]; //
				}
			}
			result=findmin(array);
			if(result[0]<routingtable[id][a])
			{
				routingtable[id][a]=result[0];
				ndata[id].ninfo.nexthop[a]=result[1];
			}
			for(l=0;l<6;l++)
			{
			array[l]=9999999;
			}
		}
	}//for end
	free(result);

}
void DVT()
{

	fd_set readfd,masterfd;
	int maxfd,result;
	int x;
	char sockp[20];
	sprintf(sockp,"%d",myport);
	char* sockport;
	sockport=sockp;
	sockfd=setup_udp(sockport);
	struct sockaddr_storage ac;
	ssize_t cnt;
	void *packetptr;
	char buff[200];
	struct timeval timeout;
	timeout.tv_sec=atol(arg4);
	timeout.tv_usec=0;
	//time_t timeout=time(NULL);
	FD_ZERO(&masterfd);
	FD_ZERO(&readfd);
	FD_SET (0,&masterfd);
	FD_SET(sockfd,&masterfd);
	maxfd=sockfd;
	time_t currenttime=time(NULL);
	int i,j;
	time_t expectedtimeout=time(NULL);			//variable for the expected timeouts
	fflush(stdout);

	//http://www.lowtek.com/sockets/select.html
	timeout.tv_sec=atol(arg4); //long
	for(;;)
	{	readfd=masterfd;

		timeout.tv_usec=0;
		x=select(maxfd+1, &readfd, NULL, NULL, &timeout);   //wait till timeout

		//	There was some problem with select and hence termination
		if(x==-1)
		{
			exit(4);
		}
		//	if the timeout has occurred then the server broadcasts its distance vector to all of its neighbors
		if(x==0)
		{
			//	creates the packet to be send to all the neighboring routers
			packetptr=createpacket();

			//	send the packet to all the neighbors
			sendneighbors(packetptr);
			timeout.tv_sec=atol(arg4); //long
		}
		else
		{
			if(FD_ISSET(0,&readfd))
			{
				//compare input command
				fflush(stdin);
				fgets(buff,60,stdin);
				command(buff);
				fflush(stdout);
			}
			else if(FD_ISSET(sockfd,&readfd))
			{

			//	printf("Entering the receivepacket()");
				//		On receiving the update packet this will start working and the packet will be received and the routing table will be updated.

				receivepacket();
				packetnum++;

				//		distance vector is calculated from the routing table.. the distance vector is calculated for the current id and algorithm for that.
				Bellmanford();

			}
			else
			{

			}	//if end
		}//else end
		//time_t currenttime=time(NULL);
	}//for end

}//DVT end



