#include "dw_ethernet.h"

// Note: 
//		ETH_ALT_PHY_IF_INIT		is called once (Addr, P1 & P2 are irrelevent)
//									Addr == 0
//									P1   == 0
//									P2   == 0
//		ETH_ALT_PHY_IF_CFG		is called only for all valid Addr
//									Addr == Address of the PHY
//									P1   == Rate (same values as the Rate argiment of PHY_init()
//									P2   == 0
//		ETH_ALT_PHY_IF_REG_RD	is called for all Addr
//									Addr == Address of the PHY
//									P1   == register # to read
//									P2   == 0
//								When a PHY Addr is for a real PHY:
//									All IEEE standard registers can be accessed
//									IEEE are 16 bit registers
//								When a PHY Addr is for a non-existent PHY (except if "master PHY"):
//									Only called for registers #2 & #3.
//									The value returned MUST be 0xFFFF for non-existent
//									The value returned MUST be 1 for non-existent "master PHY"
//		ETH_ALT_PHY_IF_REG_WRT	is called only for valid Addr
//									Addr == Address of the PHY
//									P1   == register # to write
//									P2   == value to write (only the lower 16  bits)
//								All IEEE standard registers can be accessed
//		ETH_ALT_PHY_IF_GET_RATE is called only for valid Addr
//									Addr == Address of the PHY
//									P1   == 0
//									P2   == 0
//
// The "master PHY" may or not be a real PHY. It is needed by the driver to set-up the RGMII bus rate.
// Although there may not be a real PHY when Addr matches taht "master PHY", must retyurn an ID
// that is not 0 or 0xFFF0FFFF.  return 1 is an easy choice.
// The "master PHY" is considered as a valid PHY by the driver so it will set-it up as all other
// PHYs.  If there isn't a real PHY attached toit, register #2 & #3 read should return a valeu of 1
//				

int AltPhyIF(int Dev, int Cmd, int Addr, int P1, int P2)
{
int RetVal;

	RetVal = 0;

	switch(Cmd) {
	case ETH_ALT_PHY_IF_INIT:
		if (Dev == 1234) {							/* When is an Alternate PHY i/F					*/
			// Init the communication. e.g SPIinit I2Cinit etc
			// Perform all initial set-up
			// Addr is always 0 with this command
			RetVal = 1;
		}
		break;
	case ETH_ALT_PHY_IF_CFG:
		// device configuration, e,g, setting up clock dat skew
		break;
	case ETH_ALT_PHY_IF_REG_RD:
		// P1 is the register # to read
		// 2 & 3 PHY ID - When invalid PHY return 0xFFFF (same ret value for both ID regs)
		break;
	case ETH_ALT_PHY_IF_REG_WRT:
		// P1 is the register #
		// P2 is the 16 bit value to write
		// Set RetVal to 1 in case of error or timeout
		break;
	case ETH_ALT_PHY_IF_GET_RATE:
		// Put in RetVal the auto-negotiation resulting rate - same values as PHY_init() returns
		break;
	default:
		break;
	}

	return(RetVal);
}
