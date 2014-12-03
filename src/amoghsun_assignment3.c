/**
 * @amoghsun_assignment3
 * @author  Amogh Sunil Antarkar <amoghsun@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "../include/server.h"
#include "../include/global.h"
#include "../include/logger.h"

const char* arg2;
const char* arg4;
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */


int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log();

	/*Clear LOGFILE and DUMPFILE*/
	fclose(fopen(LOGFILE, "w"));
	fclose(fopen(DUMPFILE, "wb"));

	char *arg1=argv[1];	//-t
	arg2=argv[2]; 	//path
	char *arg3=argv[3];	//-i
	arg4=argv[4];	//update interval
	/*Start Here*/
	char *s1="-t",*s3="-i";

	if((strcmp(argv[1],s1)==0)&&strcmp(argv[3],s3)==0)	/*if -t and -i argument input correctly*/
	{
		/*This function extracts the topology file content and sets up the initial routing table as acc to Dist Vect Algo specifications*/
			routing_init();

		/*select loop , UDP socket creation,Bellman ford equation  and timer logic*/
			DVT();
	}
	else
	{
		printf("Incorrect Usage\n Correct Usage format:./assignment3 ­-t <path­to­topology­file> -­i <routing­update­interval> ");
	}



	return 0;
}
