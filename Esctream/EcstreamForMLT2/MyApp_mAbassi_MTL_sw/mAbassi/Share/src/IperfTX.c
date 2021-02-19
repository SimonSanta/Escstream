/* ------------------------------------------------------------------------------------------------ */
/* FILE :		IperfTX.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				RTOS iperf test with TCP/IP.														*/
/*				The target device (me) sends TCP/IP packects.										*/
/*				The computer running iperf is a server and for example the command to use on the	*/
/*				computer is:																		*/
/*							     iperf -s -i 1														*/
/*																									*/
/*				Start iperf on the conputer before running iperf on me								*/
/*																									*/
/*				MAKE SURE TO SET THE iperf HOST IP ADDRESS											*/
/*						set the followings:															*/
/*								 TXPERF_ADDR0														*/
/*								 TXPERF_ADDR1														*/
/*								 TXPERF_ADDR2														*/
/*								 TXPERF_ADDR3														*/
/*				        and, if iperf on the conputer is not set to use the default port (5001):	*/
/*				                 TXPERF_PORT														*/
/*																									*/
/*				See the comments for TXPERF_CON_TYPE to change the type of lwIP connection			*/
/*																									*/
/*				Add this file in the build and when the IP link is up and running start the iperf	*/
/*				task by calling:																	*/
/*					             IperfTXinit();														*/
/*																									*/
/*	NOTE:																							*/
/*				Was tested with iperf V2.0															*/
/*																									*/
/* Copyright (c) 2019, Code-Time Technologies Inc. All rights reserved.								*/
/*																									*/
/* Code-Time Technologies retains all right, title, and interest in and to this work				*/
/*																									*/
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS							*/
/* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF										*/
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL							*/
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR								*/
/* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,							*/
/* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR							*/
/* OTHER DEALINGS IN THE SOFTWARE.																	*/
/*																									*/
/*																									*/
/*	$Revision: 1.5 $																				*/
/*	$Date: 2019/02/10 16:54:30 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include "lwipopts.h"
#include "WebApp.h"

/* ------------------------------------------------------------------------------------------------ */
/* IP address & port of the host running iperf 														*/
/* The IP address is IPERF_ADDR3.IPERF_ADDR2.IPERF_ADDR1.IPERF_ADDR0								*/

#ifndef TXPERF_ADDR3
  #define TXPERF_ADDR3		192
#endif
#ifndef TXPERF_ADDR2
  #define TXPERF_ADDR2		168
#endif
#ifndef TXPERF_ADDR1
  #define TXPERF_ADDR1		1
#endif
#ifndef TXPERF_ADDR0
  #define TXPERF_ADDR0		32
#endif
#ifndef TXPERF_PORT
  #define TXPERF_PORT		5001
#endif

/* ------------------------------------------------------------------------------------------------ */
/* If using netconn, Raw, or BSD sockests															*/
/* Netconn          - set to 0																		*/
/* Raw with callbck - set to 1																		*/
/* BSD sockets      - set to any other value than 0 or 1											*/

#ifndef TXPERF_CON_TYPE
  #define TXPERF_CON_TYPE	0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* How to deal with the buffer use as the data source for the packet								*/
/* Binary ORing of these (lwIP description):														*/
/* TCP_WRITE_FLAG_COPY:																				*/
/*		indicates that lwIP should allocate new memory and copy the data into it. If not specified,	*/
/*		no new memory should be allocated and the data should only be referenced by pointer.		*/
/* TCP_WRITE_FLAG_MORE (TCP only):																	*/
/*		Indicates that the push flag should not be set in the TCP segment. 							*/
/*		- The push flag is sort of a priority indicator informing sender & receiver to immediately	*/
/*		forward the packet																			*/

#ifndef TXPERF_COPY
  #define TXPERF_COPY		((TCP_WRITE_FLAG_COPY)|(TCP_WRITE_FLAG_MORE))
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Size of each TCP payload sent.																	*/
/* For TCP, the larger the better the throughput will be. Netconn is not ffected as mush as BSD		*/
/* sockets, i.e. BSD sockets need much larger payload for a good throughput							*/

#ifndef TXPERF_BUFSIZE
  #define TXPERF_BUFSIZE	(32 * TCP_MSS)
#endif

static char g_Buffer[TXPERF_BUFSIZE];
static ip_addr_t g_IPaddr;

/* ------------------------------------------------------------------------------------------------ */

void IperfTXtask(void);

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

void IperfTXinit(void)
{
	IP4_ADDR(&g_IPaddr, TXPERF_ADDR3, TXPERF_ADDR2, TXPERF_ADDR1, TXPERF_ADDR0);

	TSKcreate("TXperf", 0, 16384, IperfTXtask, 1);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Netconn																							*/
/* ------------------------------------------------------------------------------------------------ */

#if ((TXPERF_CON_TYPE) == 0)

void IperfTXtask(void)
{
struct netconn *Conn;

	Conn = netconn_new(NETCONN_TCP);
    if (Conn == NULL) {
    	printf("IperfTX: cannot create connection\n");
		TSKselfSusp();
    }

    if (netconn_connect(Conn, &g_IPaddr, TXPERF_PORT) != ERR_OK) {
		printf("IperfTX: cannot connect to iperf server\n");
		TSKselfSusp();
	}

	printf("IperfTX: connected\n");

    for (;;) {
    	netconn_write(Conn, &g_Buffer[0], sizeof(g_Buffer), TXPERF_COPY);
	}
}

/* ------------------------------------------------------------------------------------------------ */
/* Raw																								*/
/* ------------------------------------------------------------------------------------------------ */

#elif ((TXPERF_CON_TYPE) == 1)

static err_t RawSentCB(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	tcp_write(tpcb, g_Buffer, TXPERF_BUFSIZE, TXPERF_COPY);
	tcp_output(tpcb);

	return(ERR_OK);
}

/* ------------------------------------------------------------------------------------------------ */


static err_t RawConnCB(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	printf("IperfTX: connected\n");

	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, RawSentCB);
    tcp_write(tpcb, &g_Buffer[0], sizeof(g_Buffer), TXPERF_COPY);
    tcp_output(tpcb);

	return(ERR_OK);
}

/* ------------------------------------------------------------------------------------------------ */

void IperfTXtask(void) 
{

	if (tcp_connect(tcp_new(), &g_IPaddr, TXPERF_PORT, RawConnCB) != ERR_OK) {
    	printf("IperfTX: cannot create connection\n");
	}

	TSKselfSusp();
}

/* ------------------------------------------------------------------------------------------------ */
/* BSD sockets																						*/
/* ------------------------------------------------------------------------------------------------ */

#else
 #if ((LWIP_SOCKET) == 0)
  #error "IperfTX - LWIP_SOCKET must be set to 1 for socket test (Demos 11,13,15,17,19)"
 #endif

void IperfTXtask(void) 
{
char               AddrStr[16];
struct sockaddr_in Server;
int                Sock;


	Sock = socket(AF_INET, SOCK_STREAM, 0);			/* Create a new TCP socket						*/
	if (Sock < 0) {
    	printf("IperfTX: cannot create socket\n");
		TSKselfSusp();
	}

	sprintf(&AddrStr[0], "%d.%d.%d.%d", TXPERF_ADDR3,TXPERF_ADDR2,TXPERF_ADDR1,TXPERF_ADDR0);
	Server.sin_addr.s_addr = inet_addr(&AddrStr[0]);
	Server.sin_family      = AF_INET;
	Server.sin_port        = htons(TXPERF_PORT);

    if (connect(Sock , (struct sockaddr *)&Server , sizeof(Server)) < 0) {
    	printf("IperfTX: connection error\n");
		TSKselfSusp();
    }
     
	printf("IperfTX: connected to %s\n", &AddrStr[0]);

	for(;;) {
        send(Sock , &g_Buffer[0], sizeof(g_Buffer), 0);
	}
}

#endif

/* EOF */
