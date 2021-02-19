/* ------------------------------------------------------------------------------------------------ */
/* FILE :		IperfRX.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				RTOS iperf test with TCP/IP.														*/
/*				The target device (me) receive TCP/IP packects.										*/
/*				The computer running iperf is a server and for example the command to use on the	*/
/*				computer is:																		*/
/*							     iperf -c MY_IP_ADDR -i 1 											*/
/*																									*/
/*				Start iperf on me before running iperf on the conputer								*/
/*																									*/
/*				If iperf on the conputer is not set to use the default port (5001) set:				*/
/*				                 RXPERF_PORT														*/
/*																									*/
/*				See the comments for TXPERF_CON_TYPE to change the type of lwIP connection			*/
/*																									*/
/*				Add this file in the build and when the IP link is up and running start the iperf	*/
/*				task by calling:																	*/
/*					             IperfRXinit();														*/
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
/*	$Revision: 1.3 $																				*/
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
/* Port of the host running iperf			 														*/

#ifndef RXPERF_PORT
  #define RXPERF_PORT		5001
#endif

/* ------------------------------------------------------------------------------------------------ */
/* If using netconn, Raw, or BSD sockests															*/
/* Netconn           - set to 0																		*/
/* Raw with callback - set to 1																		*/
/* BSD sockets       - set to any other value than 0 or 1											*/

#ifndef RXPERF_CON_TYPE
  #define RXPERF_CON_TYPE	0
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Size of buffer used with sockets																	*/

#ifndef RXPERF_BUFSIZE
  #define RXPERF_BUFSIZE	(32 * TCP_MSS)
#endif

#if ((RXPERF_CON_TYPE) == 2)
  static char g_Buffer[RXPERF_BUFSIZE];
#endif

/* ------------------------------------------------------------------------------------------------ */

void IperfRXtask(void);

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------ */

void IperfRXinit(void)
{

	TSKcreate("RXperf", 0, 16384, IperfRXtask, 1);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* Netconn																							*/
/* ------------------------------------------------------------------------------------------------ */

#if ((RXPERF_CON_TYPE) == 0)

void IperfRXtask(void)
{
struct netconn *Conn;
struct netconn *MyConn;
struct netbuf  *Rqst;

	Conn = netconn_new(NETCONN_TCP);
    if (Conn == NULL) {
    	printf("IperfRX: cannot create connection\n");
		TSKselfSusp();
    }

    if (netconn_bind(Conn, NULL, RXPERF_PORT) != ERR_OK) {
		printf("IperfRX: cannot bind to iperf clien\n");
		TSKselfSusp();
	}

	if (netconn_listen(Conn) != ERR_OK) {
		printf("IperfRX: cannot listen\n");
		TSKselfSusp();
	}

	netconn_accept(Conn, &MyConn);					/* Accept any incoming connection				*/

	printf("IperfRX: connected\n");

    for (;;) {
		netconn_recv(MyConn, &Rqst);
		netbuf_delete(Rqst);
	}
}

/* ------------------------------------------------------------------------------------------------ */
/* Raw																								*/
/* ------------------------------------------------------------------------------------------------ */

#elif ((RXPERF_CON_TYPE) == 1)

static err_t RawRecvCB(void *arg, struct tcp_pcb *tpcb,  struct pbuf *Pbuf,  err_t err)
{

	if (Pbuf == NULL) {
    	printf("IperfRX: connection closed\n");
		tcp_close(tpcb);
	}
	else {
		tcp_recved(tpcb, Pbuf->tot_len);
		pbuf_free(Pbuf);
	}

	return(ERR_OK);
}

/* ------------------------------------------------------------------------------------------------ */


static err_t RawConnCB(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	printf("IperfRX: ready to receive on port %d\n", RXPERF_PORT);

	tcp_arg(tpcb, NULL);
	tcp_recv(tpcb, RawRecvCB);

	return(ERR_OK);
}

/* ------------------------------------------------------------------------------------------------ */

void IperfRXtask(void) 
{
struct tcp_pcb* tpcb;

	tpcb = tcp_new();
	if (tpcb == NULL) {
    	printf("IperfRX: cannot create tcp instance\n");
		TSKselfSusp();		
	}

	if (tcp_bind(tpcb, IP_ADDR_ANY, RXPERF_PORT) != ERR_OK) {
    	printf("IperfRX: cannot bind\n");
		TSKselfSusp();		
	}

	tpcb = tcp_listen(tpcb);
	if (tpcb == NULL) {
    	printf("IperfRX: cannot listen\n");
		TSKselfSusp();		
	}

	tcp_accept(tpcb, RawConnCB);

	TSKselfSusp();
}

/* ------------------------------------------------------------------------------------------------ */
/* BSD sockets																						*/
/* ------------------------------------------------------------------------------------------------ */

#else

 #if ((LWIP_SOCKET) == 0)
  #error "IperfRX - LWIP_SOCKET must be set to 1 for socket test (Demos 11,13,15,17,19)"
 #endif

void IperfRXtask(void) 
{
struct sockaddr_in Client;
int                MyConn;
struct sockaddr_in Host;
int                Size;
int                Sock;


	Sock = socket(AF_INET, SOCK_STREAM, 0);			/* Create a new TCP socket						*/
	if (Sock < 0) {
    	printf("IperfRX: cannot create socket\n");
		TSKselfSusp();
	}

	Client.sin_addr.s_addr = htons(INADDR_ANY);
	Client.sin_family      = AF_INET;
	Client.sin_port        = htons(RXPERF_PORT);

    if (bind(Sock, (struct sockaddr *)&Client, sizeof(Client)) < 0) {
    	printf("IperfRX: binding error\n");
		TSKselfSusp();
    }

    if (listen(Sock, 5) < 0) {
    	printf("IperfRX: listen error\n");
		TSKselfSusp();
    }

	Size = sizeof(Host);
	MyConn = accept(Sock, (struct sockaddr *)&Host, (socklen_t *)&Size);
    if (MyConn < 0) {
    	printf("IperfRX: accept error\n");
		TSKselfSusp();
    }

	printf("IperfRX: ready to receive on port %d\n", RXPERF_PORT);

	for(;;) {
        recv(MyConn, &g_Buffer[0], sizeof(g_Buffer), 0);
	}
}

#endif

/* EOF */
