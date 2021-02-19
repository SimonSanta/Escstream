How to install this library:

1) Copy the source files in your project
2) In your project Makefile, include Netif_Setup_GCC.make (before the include of mAbassi's Makefile)
3) Call the functions Eth_ISR_Setup and LwIP_Init in your code

!! This library enables LwIP 1.4 (and not 2.0) !!

======= DOCUMENTATION =======

void Eth_ISR_Setup(void);
	Install the interrupt services routines needed by the Ethernet driver.

void LwIP_Init(char *addr, char *netmask, char *gateway);
	Configure the LwIP stack to use the network parameters (board IPv4 address, netmask, gateway)
	provided in the arguments as strings.

	Example: LwIP_Init("192.168.1.42", "255.255.255.0", "192.168.1.1");