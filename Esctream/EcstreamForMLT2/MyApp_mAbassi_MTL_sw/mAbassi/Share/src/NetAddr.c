/* ------------------------------------------------------------------------------------------------ */
/* FILE :		NetAddr.c																			*/
/*																									*/
/* CONTENTS :																						*/
/*				lwIP network address configuration & DHCP client task								*/
/*				DHCP client task.																	*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2019, Code-Time Technologies Inc. All rights reserved.						*/
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
/*	$Revision: 1.22 $																				*/
/*	$Date: 2019/01/10 18:07:16 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "WebApp.h"

/* ------------------------------------------------------------------------------------------------ */

#define MAX_DHCP_TRIES		10						/* Number of time to try until declared timeout	*/

#define DHCP_START			0						/* States of the DHCP process					*/
#define DHCP_WAIT_ADDRESS	1
#define DHCP_DONE			2

#if ((MY_LCD_NMB_LINE) >= 10)
  #define LCD_IPADD_LINE1	8
  #define LCD_IPADD_LINE2	9
#elif ((MY_LCD_NMB_LINE) >= 2)
  #define LCD_IPADD_LINE1	0
  #define LCD_IPADD_LINE2	1
#else
  #define LCD_IPADD_LINE1	0
  #define LCD_IPADD_LINE2	0
#endif

/* ------------------------------------------------------------------------------------------------ */

u32_t G_IPnetDefGW;									/* Default static default gateway address		*/
u32_t G_IPnetDefIP;									/* Default static default IP address			*/
u32_t G_IPnetDefNM;									/* Default net mask								*/
int   G_IPnetStatic;								/* If using static or dynamic IP address		*/
													/* All of the above are set-up by main task		*/
static struct netif g_NetIF;						/* lwIP network interface descriptor			*/

/* ------------------------------------------------------------------------------------------------ */

void GetMACaddr(int Dev, u8_t MACaddr[NETIF_MAX_HWADDR_LEN])
{
	if (Dev == 0) {
		MACaddr[0]  = MAC0_ADDR0;
		MACaddr[1]  = MAC0_ADDR1;
		MACaddr[2]  = MAC0_ADDR2;
		MACaddr[3]  = MAC0_ADDR3;
		MACaddr[4]  = MAC0_ADDR4;
		MACaddr[5]  = MAC0_ADDR5;
	}
	else if (Dev == 1) {
		MACaddr[0]  = MAC1_ADDR0;
		MACaddr[1]  = MAC1_ADDR1;
		MACaddr[2]  = MAC1_ADDR2;
		MACaddr[3]  = MAC1_ADDR3;
		MACaddr[4]  = MAC1_ADDR4;
		MACaddr[5]  = MAC1_ADDR5;
	}
	else if (Dev == 2) {
		MACaddr[0]  = MAC2_ADDR0;
		MACaddr[1]  = MAC2_ADDR1;
		MACaddr[2]  = MAC2_ADDR2;
		MACaddr[3]  = MAC2_ADDR3;
		MACaddr[4]  = MAC2_ADDR4;
		MACaddr[5]  = MAC2_ADDR5;
	}
	else if (Dev == 3) {
		MACaddr[0]  = MAC3_ADDR0;
		MACaddr[1]  = MAC3_ADDR1;
		MACaddr[2]  = MAC3_ADDR2;
		MACaddr[3]  = MAC3_ADDR3;
		MACaddr[4]  = MAC3_ADDR4;
		MACaddr[5]  = MAC3_ADDR5;
	}
	else {
		for(;;);
	}
	return;
}

/* ------------------------------------------------------------------------------------------------ */

void NetAddr_Init(void)
{
char AddrTxt[32];

	G_IPnetStatic = 0;								/* By default, use DHCP							*/
	G_IPnetDefIP  = IP4_INT32(IP_ADDR0,      IP_ADDR1,      IP_ADDR2,      IP_ADDR3);
	G_IPnetDefNM  = IP4_INT32(NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
	G_IPnetDefGW  = IP4_INT32(GATEWAY_ADDR0, GATEWAY_ADDR1, GATEWAY_ADDR2, GATEWAY_ADDR3);

	printf("\n\nDefault settings:\n");
	printf("IP address : "); PrtAddr(&AddrTxt[0], G_IPnetDefIP); puts(&AddrTxt[0]);
	printf("Net Mask   : "); PrtAddr(&AddrTxt[0], G_IPnetDefNM); puts(&AddrTxt[0]);
	printf("Gateway    : "); PrtAddr(&AddrTxt[0], G_IPnetDefGW); puts(&AddrTxt[0]);


	return;
}

/* ------------------------------------------------------------------------------------------------ */

void LwIP_Init(void)
{
char      AddrTxt[32];
ip_addr_t IPaddr;
ip_addr_t NetMask;
ip_addr_t GateWay;

	ETH_ResetEMAC(EMAC_DEV);						/* Start fresh - reset & basic init				*/

	if (G_IPnetStatic == 0) {						/* Request for dynamic address					*/
		IPaddr.addr  = 0;							/* DHCP task will run when configuration done	*/
		NetMask.addr = 0;
		GateWay.addr = 0;
	}
	else {											/* Request for static address					*/
		IPaddr.addr  = G_IPnetDefIP;
		NetMask.addr = G_IPnetDefNM;
		GateWay.addr = G_IPnetDefGW;
		MTXLOCK_STDIO();
		fputs("Static IP address : ", stdout);
		memset(&AddrTxt[0], ' ', sizeof(AddrTxt));
		PrtAddr(&AddrTxt[3], G_IPnetDefIP);
		puts(&AddrTxt[3]);
		MY_LCD_CLRLINE(LCD_IPADD_LINE1);
		MY_LCD_CLRLINE(LCD_IPADD_LINE2);
		MY_LCD_LINE(LCD_IPADD_LINE1, "   My IP Address   ");
		MY_LCD_LINE(LCD_IPADD_LINE2, &AddrTxt[0]);
		puts("The Ethernet interface is ready");
		MTXUNLOCK_STDIO();

	  #ifdef MY_LED_0
		MY_LED_ON(MY_LED_0);						/* Turn ON DHCP LED								*/
	  #endif

	  #ifndef osCMSIS
	   #if ((OX_EVENTS) != 0)
		EVTset(TSKgetID("A&E"), 1);					/* Report to Adam & Eve the Ethernet is ready	*/
	   #else
		MTXlock(G_OSmutex, -1);
		G_WebEvents |= 1;
		MTXunlock(G_OSmutex);
	   #endif
	  #else
		extern osThreadId G_TaskMainID;
		osSignalSet(G_TaskMainID, 1);
	  #endif
	}

	tcpip_init(NULL, NULL);	
	udp_init();	

	etharp_init();
	etharp_tmr();

	g_NetIF.name[0] = 'C';							/* You can use any 2 letters here as long as	*/
	g_NetIF.name[1] = 'T';							/* none of the 2 is '\0'						*/
													/* If using SLIP, see note below				*/
	netif_add(&g_NetIF, &IPaddr, &NetMask, &GateWay, NULL, ethernet_init[EMAC_DEV], &tcpip_input);

	netif_set_default(&g_NetIF);					/* This is the default network interface		*/

	netif_set_up(&g_NetIF);							/* Doc states this must be called				*/

	return;
}

/* NOTE ON SLIP:																					*/
/*		The lwIP SLIP normaly extracts the serial port number from netif->num. unless netif->state	*/
/*		is a non-NULL pointer.  Because netif->num is used to identify the EMAC I/F, to use lwIP	*/
/*		SLIP you have to pass the serial port number with a pointer to to a uint_8 variable holding	*/
/*		the number. The pointer in netif->state is specified by the NULL in the arguiments of		*/
/*		 netif_add() called above.																	*/

/* ------------------------------------------------------------------------------------------------ */
#ifdef osCMSIS
  void LwIP_DHCP_task(const void *Arg)
#else
  void LwIP_DHCP_task(void)
#endif
{
char      AddrTxt[32];
int       DHCPstate;
ip_addr_t IPaddr;
u32_t     DynamicIP;
ip_addr_t GateWay;
ip_addr_t NetMask;
int       Toggle; 

  #ifdef osCMSIS
	Arg       = Arg;
  #endif

	DHCPstate = DHCP_START;
	Toggle    = 0;

	for (;;) {
		switch (DHCPstate) {
		case DHCP_START:
			MTXLOCK_STDIO();
			fputs("Waiting for DHCP server\r", stdout);
			MY_LCD_CLRLINE(LCD_IPADD_LINE1);
			MY_LCD_CLRLINE(LCD_IPADD_LINE2);
		  #if ((MY_LCD_WIDTH) < 16)
			MY_LCD_LINE(LCD_IPADD_LINE1, "Waiting DHCP");
		  #elif ((MY_LCD_WIDTH) < 20)
			MY_LCD_LINE(LCD_IPADD_LINE1, "Waiting for DHCP");
		  #else
			MY_LCD_LINE(LCD_IPADD_LINE1, "  Waiting for DHCP");
		  #endif
			MTXUNLOCK_STDIO();
			dhcp_start(&g_NetIF);
			DynamicIP = 0;
			DHCPstate = DHCP_WAIT_ADDRESS;
			break;

		case DHCP_WAIT_ADDRESS:
			MTXLOCK_STDIO();
			if (Toggle == 0) {						/* Blink the "waiting for DHCP" message			*/
				fputs("Waiting for DHCP server\r", stdout);
		  #if ((MY_LCD_WIDTH) < 16)
			MY_LCD_LINE(LCD_IPADD_LINE1, "Waiting DHCP");
		  #elif ((MY_LCD_WIDTH) < 20)
			MY_LCD_LINE(LCD_IPADD_LINE1, "Waiting for DHCP");
		  #else
			MY_LCD_LINE(LCD_IPADD_LINE1, "  Waiting for DHCP");
		  #endif
			  #ifdef MY_LED_0
				MY_LED_ON(MY_LED_0);
			  #endif
			}
			else {
				fputs("                       \r", stdout);
				MY_LCD_CLRLINE(LCD_IPADD_LINE1);
			  #ifdef MY_LED_0
				MY_LED_OFF(MY_LED_0);
			  #endif
			}
			MTXUNLOCK_STDIO();
			TSKsleep(OS_TICK_PER_SEC/2);

			Toggle = (Toggle+1) & 1;

			DynamicIP = g_NetIF.ip_addr.addr;		/* Grab the current address						*/
			if (DynamicIP != 0) {					/* If the address is not 0 anymore, DHCP server	*/
				MTXLOCK_STDIO();					/* gave us our address							*/
				fputs("Dynamic IP address : ", stdout);
				memset(&AddrTxt[0], ' ', sizeof(AddrTxt));
				PrtAddr(&AddrTxt[3], DynamicIP);
				puts(&AddrTxt[3]);
				MY_LCD_CLRLINE(LCD_IPADD_LINE1);
				MY_LCD_CLRLINE(LCD_IPADD_LINE2);
			  #if ((MY_LCD_WIDTH) < 20)
				MY_LCD_LINE(LCD_IPADD_LINE1, "IP Address");
				MY_LCD_LINE(LCD_IPADD_LINE2, &AddrTxt[3]);
			  #else
				MY_LCD_LINE(LCD_IPADD_LINE1, "   My IP Address   ");
				MY_LCD_LINE(LCD_IPADD_LINE2, &AddrTxt[0]);
			  #endif

				MTXUNLOCK_STDIO();
				DHCPstate = DHCP_DONE;
			}
			else {									/* Check for timeout: too long wait for reply	*/
			  #if ((LWIP_VERSION_MAJOR) == 2U)
				if (netif_dhcp_data(&g_NetIF)->tries > MAX_DHCP_TRIES) {
			  #else
				if (g_NetIF.dhcp->tries > MAX_DHCP_TRIES)	{	/* fall back to static address		*/
			  #endif
					IP4_ADDR(&IPaddr,  IP_ADDR0,      IP_ADDR1,      IP_ADDR2,      IP_ADDR3 );
					IP4_ADDR(&NetMask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
					IP4_ADDR(&GateWay, GATEWAY_ADDR0, GATEWAY_ADDR1, GATEWAY_ADDR2, GATEWAY_ADDR3);

					netif_set_addr(&g_NetIF, &IPaddr , &NetMask, &GateWay);

					MTXLOCK_STDIO();
					fputs("DHCP timeout, fallback to static address : ", stdout);
					memset(&AddrTxt[0], ' ', sizeof(AddrTxt));
					PrtAddr(&AddrTxt[3], G_IPnetDefIP);
					puts(&AddrTxt[3]);
					MY_LCD_CLRLINE(LCD_IPADD_LINE1);
					MY_LCD_CLRLINE(LCD_IPADD_LINE2);
				  #if ((MY_LCD_WIDTH) < 20)
					MY_LCD_LINE(LCD_IPADD_LINE1, "IP Address");
					MY_LCD_LINE(LCD_IPADD_LINE2, &AddrTxt[3]);
				  #else
					MY_LCD_LINE(LCD_IPADD_LINE1, "   My IP Address   ");
					MY_LCD_LINE(LCD_IPADD_LINE2, &AddrTxt[0]);
				  #endif
					MTXUNLOCK_STDIO();
					DHCPstate = DHCP_DONE;
				}
			}
			break;

		case DHCP_DONE:
			MTXLOCK_STDIO();
			puts("The Ethernet interface is ready");
			MTXUNLOCK_STDIO();
		  #ifdef MY_LED_0
			MY_LED_ON(MY_LED_0);
		  #endif
		  #ifndef osCMSIS
		   #if ((OX_EVENTS) != 0)
			EVTset(TSKgetID("A&E"), 1);				/* Report to Adam & Eve the Ethernet is ready	*/
		   #else
			MTXlock(G_OSmutex, -1);
			G_WebEvents |= 1;
			MTXunlock(G_OSmutex);
		   #endif
		  #else
			extern osThreadId G_TaskMainID;
			osSignalSet(G_TaskMainID, 1);
		  #endif
		  #ifndef osCMSIS
			TSKselfSusp();							/* Done. If resumed, will restart from scratch	*/
		  #else
			osThreadTerminate(osThreadGetId());
		  #endif
			dhcp_release(&g_NetIF);					/* Restart means to release and then to stop	*/
			dhcp_stop(&g_NetIF);					/* After stop, we can ask again for an address	*/
			DHCPstate = DHCP_START;
			break;

		default:
			break;
		}

		if (DHCPstate == DHCP_WAIT_ADDRESS) {		/* Sleep only when  waiting for DHCP server		*/
		  #ifndef osCMSIS
			TSKsleep(OS_MS_TO_TICK(250));			/* Go to sleep for 1/4 second					*/
		  #else
			osDelay(250);							/* Go to sleep for 1/4 second					*/
		  #endif
		}
  	}
}

/* ------------------------------------------------------------------------------------------------ */

void PrtAddr(char *Str, u32_t Addr)
{
u8_t *Baddr;

	Baddr = (u8_t *)&Addr;
  #if BYTE_ORDER == BIG_ENDIAN
	sprintf(Str, "%d.%d.%d.%d", Baddr[3], Baddr[2], Baddr[1], Baddr[0]);
  #else
	sprintf(Str, "%d.%d.%d.%d", Baddr[0], Baddr[1], Baddr[2], Baddr[3]);
  #endif
	return;
}


/* EOF */
