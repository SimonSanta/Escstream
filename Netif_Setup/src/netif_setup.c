#include "netif_setup.h"
#include "lwip/inet.h"
													/* All of the above are set-up by main task		*/
static struct netif g_NetIF;						/* lwIP network interface descriptor			*/

void Eth_ISR_Setup(void)
{
#if (0U == ((ETH_LIST_DEVICE) & (1U << (EMAC_DEV))))
	#error "Selected EMAC device is not in ETH_LIST_DEVICE"
#endif

#if (((ETH_LIST_DEVICE) & 0x01U) != 0U)
	OSisrInstall(EMAC0_INT, Emac0_IRQHandler);		/* Install the Ethernet interrupt for EMAC #0	*/
	GICenable(EMAC0_INT, 128, 0);					/* Enable the EMAC #0 interrupts (Level)		*/
#endif

#if (((ETH_LIST_DEVICE) & 0x02U) != 0U)
	OSisrInstall(EMAC1_INT, Emac1_IRQHandler);		/* Install the Ethernet interrupt for EMAC #1	*/
	GICenable(EMAC1_INT, 128, 0);					/* Enable the EMAC #1 interrupst (Level)		*/
#endif

#if (((ETH_LIST_DEVICE) & 0x04U) != 0U)
	OSisrInstall(EMAC2_INT, Emac2_IRQHandler);		/* Install the Ethernet interrupt for EMAC #2	*/
	GICenable(EMAC2_INT, 128, 0);					/* Enable the EMAC #2 interrupts (Level)		*/
#endif

#if (((ETH_LIST_DEVICE) & 0x08U) != 0U)
	OSisrInstall(EMAC3_INT, Emac3_IRQHandler);		/* Install the Ethernet interrupt for EMAC #2	*/
	GICenable(EMAC3_INT, 128, 0);					/* Enable the EMAC #3 interrupts (Level)		*/
#endif
}

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

void LwIP_Init(char *addr, char *netmask, char *gateway)
{
	ip_addr_t IPaddr;
	ip_addr_t NetMask;
	ip_addr_t GateWay;

	IPaddr.addr = inet_addr(addr);
	NetMask.addr = inet_addr(netmask);
	GateWay.addr = inet_addr(gateway);
	if (IPaddr.addr == INADDR_NONE || NetMask.addr == INADDR_NONE || GateWay.addr == INADDR_NONE) {
		printf("Incorrect parameters supplied to LwIP_Init.\n");
		return;
	}

	MTXLOCK_STDIO();
	printf("[LwIP] Configuring with static address %s ...\n", addr);
	MTXUNLOCK_STDIO();

	ETH_ResetEMAC(EMAC_DEV);						/* Start fresh - reset & basic init				*/
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
	TSKsleep(OS_MS_TO_TICK(1000));

	MTXLOCK_STDIO();
	printf("[LwIP] Done.\n");
	MTXUNLOCK_STDIO();
	return;
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
