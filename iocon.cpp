#include <iostream>

#include "types.hpp"
#include "iocon.hpp"

namespace IOCON {

u32 PIO0_0,	// R/W 0x000 configuration for pin RESET/PIO0_0
    PIO0_1,	// R/W 0x004 configuration for pin PIO0_1/CLKOUT/CT32B0_MAT2/USB_FTOGGLE
    PIO0_2,	// R/W 0x008 configuration for pin PIO0_2/SSP0_SSEL/CT16B0_CAP0/R_0
    PIO0_3,	// R/W 0x00C configuration for pin PIO0_3/USB_VBUS/R_1
    PIO0_4,	// R/W 0x010 configuration for pin PIO0_4/I2C0_SCL/R_2
    PIO0_5,	// R/W 0x014 configuration for pin PIO0_5/I2C0_SDA/R_3
    PIO0_6,	// R/W 0x018 configuration for pin PIO0_6/R/SSP0_SCK/R_4
    PIO0_7,	// R/W 0x01C configuration for pin PIO0_7/U0_nCTS/R_5/I2C1_SCL
    PIO0_8,	// R/W 0x020 configuration for pin PIO0_8/SSP0_MISO/CT16B0_MAT0/R/R_6
    PIO0_9,	// R/W 0x024 configuration for pin PIO0_9/SSP0_MOSI/CT16B0_MAT1/R/R_7
    PIO0_10, 	// R/W 0x028 configuration for pin SWCLK/PIO0_10/SSP0_SCK/CT16B0_MAT2
    PIO0_11, 	// R/W 0x02C configuration for pin TDI/PIO0_11/ADC_9/CT32B0_MAT3/U1_nRTS/U1_SCLK
    PIO0_12, 	// R/W 0x030 I/O configuration for pin TMS/PIO0_12/ADC_8/CT32B1_CAP0/U1_nCTS
    PIO0_13, 	// R/W 0x034 I/O configuration for pin TDO/PIO0_13/ADC_7/CT32B1_MAT0/U1_RXD
    PIO0_14, 	// R/W 0x038 I/O configuration for pinnTRST/PIO0_14/ADC_6/CT32B1_MAT1/U1_TXD
    PIO0_15, 	// R/W 0x03C I/O configuration for pinSWDIO/PIO0_15/ADC_3/CT32B1_MAT2
    PIO0_16,	// R/W 0x040 I/O configuration for pinPIO0_16/ADC_2/CT32B1_MAT3/R_8/WAKEUP
    PIO0_17,	// R/W 0x044 I/O configuration for pinPIO0_17/U0_nRTS/CT32B0_CAP0/U0_SCLK
    PIO0_18,	// R/W 0x048 I/O configuration for pinPIO0_18/U0_RXD/CT32B0_MAT0
    PIO0_19,	// R/W 0x04C I/O configuration for pinPIO0_19/U0_TXD/CT32B0_MAT1
    PIO0_20,	// R/W 0x050 I/O configuration for pinPIO0_20/CT16B1_CAP0/U2_RXD
    PIO0_21, 	//  R/W 0x054 I/O configuration for pin PIO0_21/CT16B1_MAT0/SSP1_MOSI 
    PIO0_22, 	//  R/W 0x058 I/O configuration for pin PIO0_22/ADC_11/CT16B1_CAP1/SSP1_MISO 
    PIO0_23, 	//  R/W 0x05C I/O configuration for pin PIO0_23/ADC_1/R_9/U0_nRI/SSP1_SSEL 
    PIO1_0, 	//  R/W 0x060 I/O configuration for pin PIO1_0/CT32B1_MAT0/R_10/U2_TXD 
    PIO1_1, 	//  R/W 0x064 PIO1_1/CT32B1_MAT1/R_11/U0_nDTR
    PIO1_2, 	//  R/W 0x068 I/O configuration for pin PIO1_2/CT32B1_MAT2/R_12/U1_RXD 
    PIO1_3, 	//  R/W 0x06C I/O configuration for pin PIO1_3/CT32B1_MAT3/R_13/I2C1_SDA/ADC_5 
    PIO1_4, 	//  R/W 0x070 I/O configuration for pin PIO1_4/CT32B1_CAP0/R_14/U0_nDSR 
    PIO1_5, 	//  R/W 0x074 I/O configuration for pin PIO1_5/CT32B1_CAP1/R_15/U0_nDCD 
    PIO1_6, 	//  R/W 0x078 I/O configuration for pin PIO1_6/R_16/U2_RXD/CT32B0_CAP2 
    PIO1_7, 	//  R/W 0x07C I/O configuration for pin PIO1_7/R_17/U2_nCTS/CT16B1_CAP0 
    PIO1_8, 	//  R/W 0x080 I/O configuration for pin PIO1_8/R_18/U1_TXD/CT16B0_CAP0 
    PIO1_9, 	//  R/W 0x084 I/O configuration for pin PIO1_9/U0_nCTS/CT16B1_MAT1/ADC_0 
    PIO1_10, 	//  R/W 0x088 I/O configuration for pin PIO1_10/U2_nRTS/U2_SCLK/CT16B1_MAT0 
    PIO1_11, 	//  R/W 0x08C I/O configuration for pin PIO1_11/I2C1_SCL/CT16B0_MAT2//U0_nRI 
    PIO1_12, 	//  R/W 0x090 I/O configuration for pin PIO1_12/SSP0_MOSI/CT16B0_MAT1/R_21 
    PIO1_13, 	//  R/W 0x094 I/O configuration for pin PIO1_13/U1_nCTS/SCT0_OUT3/R_22 
    PIO1_14, 	//  R/W 0x098 I/O configuration for pin PIO1_14/I2C1_SDA/CT32B1_MAT2/R_23 
    PIO1_15, 	//  R/W 0x09C I/O configuration for pin PIO1_15/SSP0_SSEL/CT32B1_MAT3/R_24 
    PIO1_16, 	//  R/W 0x0A0 I/O configuration for pin PIO1_16/SSP0_MISO/CT16B0_MAT0/R_25 
    PIO1_17, 	//  R/W 0x0A4 I/O configuration for pin PIO1_17/CT16B0_CAP2/U0_RXD/R_26
    PIO1_18, 	//  R/W 0x0A8 I/O configuration for pin PIO1_18/CT16B1_CAP1/U0_TXD/R_27
    PIO1_19, 	//  R/W 0x0AC I/O configuration for pin PIO1_19/U2_nCTS/SCT0_OUT0/R_28 
    PIO1_20, //  R/W 0x0B0 I/O configuration for pin PIO1_20/U0_nDSR/SSP1_SCK/CT16B0_MAT0
    PIO1_21, //  R/W 0x0B4 I/O configuration for pin PIO1_21/U0_nDCD/SSP1_MISO/CT16B0_CAP2
    PIO1_22, //  R/W 0x0B8 I/O configuration for pin PIO1_22/SSP1_MOSI/CT32B1_CAP1/ADC_4/R_29
    PIO1_23, //  R/W 0x0BC I/O configuration for pin PIO1_23/CT16B1_MAT1/SSP1_SSEL/U2_TXD
    PIO1_24, //  R/W 0x0C0 I/O configuration for pin PIO1_24/CT32B0_MAT0/I2C1_SDA
    PIO1_25, //  R/W 0x0C4 I/O configuration for pin PIO1_25/U2_nRTS/U2_SCLK/SCT0_IN0/R_30
    PIO1_26, //  R/W 0x0C8 I/O configuration for pin PIO1_26/CT32B0_MAT2/U0_RXD/R_19
    PIO1_27, //  R/W 0x0CC I/O configuration for pin PIO1_27/CT32B0_MAT3/U0_TXD/R_20/SSP1_SCK
    PIO1_28, //  R/W 0x0D0 I/O configuration for pin PIO1_28/CT32B0_CAP0/U0_SCLK/U0_nRTS
    PIO1_29, //  R/W 0x0D4 I/O configuration for pin PIO1_29/SSP0_SCK/CT32B0_CAP2/U0_nDTR
    PIO1_30, //  R/W 0x0D8 I/O configuration for pin PIO1_30/I2C1_SCL/SCT0_IN3/R_31
    PIO1_31, //  R/W 0x0DC I/O configuration for pin PIO1_31
    PIO2_0, //  R/W 0x0F0 I/O configuration for pin PIO2_0/XTALIN
    PIO2_1, //  R/W 0x0F4 I/O configuration for pin PIO2_1/XTALOUT
    PIO2_2, //  R/W 0x0FC I/O configuration for pin PIO2_2/U3_nRTS/U3_SCLK/SCT0_OUT1
    PIO2_3, //  R/W 0x100 I/O configuration for pin PIO2_3/U3_RXD/CT32B0_MAT1
    PIO2_4, //  R/W 0x104 I/O configuration for pin PIO2_4/U3_TXD/CT32B0_MAT2
    PIO2_5, //  R/W 0x108 I/O configuration for pin PIO2_5/U3_nCTS/SCT0_IN1
    PIO2_6, //  R/W 0x10C I/O configuration for pin PIO2_6/U1_nRTS/U1_SCLK/SCT0_IN2
    PIO2_7, //  R/W 0x110 I/O configuration for pin PIO2_7/SSP0_SCK/SCT0_OUT2
    PIO2_8, //  R/W 0x114 I/O configuration for pin PIO2_8/SCT1_IN0
    PIO2_9, //  R/W 0x118 I/O configuration for pin PIO2_9/SCT1_IN1
    PIO2_10, //  R/W 0x11C I/O configuration for pin PIO2_10/U4_nRTS/U4_SCLK
    PIO2_11, //  R/W 0x120 I/O configuration for pin PIO2_11/U4_RXD
    PIO2_12, //  R/W 0x124 I/O configuration for pin PIO2_12/U4_TXD
    PIO2_13, //  R/W 0x128 I/O configuration for pin PIO2_13/U4_nCTS
    PIO2_14, //  R/W 0x12C I/O configuration for pin PIO2_14/SCT1_IN2
    PIO2_15, //  R/W 0x130 I/O configuration for pin PIO2_15/SCT1_IN3
    PIO2_16, //  R/W 0x134 I/O configuration for pin PIO2_16/SCT1_OUT0
    PIO2_17, //  R/W 0x138 I/O configuration for PIO2_17/SCT1_OUT1
    PIO2_18, //  R/W 0x13C I/O configuration for PIO2_18/SCT1_OUT2
    PIO2_19, //  R/W 0x140 I/O configuration for pin PIO2_19/SCT1_OUT3
    PIO2_20, //  R/W 0x144 I/O configuration for pin PIO2_20
    PIO2_21, //  R/W 0x148 I/O configuration for pin PIO2_21
    PIO2_22, //  R/W 0x14C I/O configuration for pin PIO2_22
    PIO2_23  //  R/W 0x150 I/O configuration for pin PIO2_23
    ;
    
    MMU::Register map[] = {
	MMUREG(PIO0_0),
	MMUREG(PIO0_1),
	MMUREG(PIO0_2),
	MMUREG(PIO0_3),
	MMUREG(PIO0_4),
	MMUREG(PIO0_5),
	MMUREG(PIO0_6),
	MMUREG(PIO0_7),
	MMUREG(PIO0_8),
	MMUREG(PIO0_9),
	MMUREG(PIO0_10),
	MMUREG(PIO0_11),
	MMUREG(PIO0_12),
	MMUREG(PIO0_13),
	MMUREG(PIO0_14),
	MMUREG(PIO0_15),
	MMUREG(PIO0_16),
	MMUREG(PIO0_17),
	MMUREG(PIO0_18),
	MMUREG(PIO0_19),
	MMUREG(PIO0_20),
	MMUREG(PIO0_21),
	MMUREG(PIO0_22),
	MMUREG(PIO0_23),
	MMUREG(PIO1_0),
	MMUREG(PIO1_1),
	MMUREG(PIO1_2),
	MMUREG(PIO1_3),
	MMUREG(PIO1_4),
	MMUREG(PIO1_5),
	MMUREG(PIO1_6),
	MMUREG(PIO1_7),
	MMUREG(PIO1_8),
	MMUREG(PIO1_9),
	MMUREG(PIO1_10),
	MMUREG(PIO1_11),
	MMUREG(PIO1_12),
	MMUREG(PIO1_13),
	MMUREG(PIO1_14),
	MMUREG(PIO1_15),
	MMUREG(PIO1_16),
	MMUREG(PIO1_17),
	MMUREG(PIO1_18),
	MMUREG(PIO1_19),
	MMUREG(PIO1_20),
	MMUREG(PIO1_21),
	MMUREG(PIO1_22),
	MMUREG(PIO1_23),
	MMUREG(PIO1_24),
	MMUREG(PIO1_25),
	MMUREG(PIO1_26),
	MMUREG(PIO1_27),
	MMUREG(PIO1_28),
	MMUREG(PIO1_29),
	MMUREG(PIO1_30),
	MMUREG(PIO1_31),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREG(PIO2_0),
	MMUREG(PIO2_1),
	MMUREGX(),
	MMUREG(PIO2_2),
	MMUREG(PIO2_3),
	MMUREG(PIO2_4),
	MMUREG(PIO2_5),
	MMUREG(PIO2_6),
	MMUREG(PIO2_7),
	MMUREG(PIO2_8),
	MMUREG(PIO2_9),
	MMUREG(PIO2_10),
	MMUREG(PIO2_11),
	MMUREG(PIO2_12),
	MMUREG(PIO2_13),
	MMUREG(PIO2_14),
	MMUREG(PIO2_15),
	MMUREG(PIO2_16),
	MMUREG(PIO2_17),
	MMUREG(PIO2_18),
	MMUREG(PIO2_19),
	MMUREG(PIO2_20),
	MMUREG(PIO2_21),
	MMUREG(PIO2_22),
	MMUREG(PIO2_23)
    };

    MMU::Layout layout = {
	0x40044000,
	sizeof(map) / sizeof(map[0]),
	map
    };

    void init(){

	PIO0_0 = 0x90; 
	PIO0_1 = 0x90; 
	PIO0_2 = 0x90; 
	PIO0_3 = 0x90; 
	PIO0_4 = 0x90; 
	PIO0_5 = 0x90; 
	PIO0_6 = 0x90; 
	PIO0_7 = 0x90; 
	PIO0_8 = 0x90; 
	PIO0_9 = 0x90; 
	PIO0_10 = 0x90; 
	PIO0_11 = 0x90; 
	PIO0_12 = 0x90; 
	PIO0_13 = 0x90; 
	PIO0_14 = 0x90; 
	PIO0_15 = 0x90; 
	PIO0_16 = 0x90; 
	PIO0_17 = 0x90; 
	PIO0_18 = 0x90; 
	PIO0_19 = 0x90; 
	PIO0_20 = 0x90; 
	PIO0_21 = 0x90; 
	PIO0_22 = 0x90; 
	PIO0_23 = 0x90; 
	PIO1_0 = 0x90; 
	PIO1_1 = 0x90; 
	PIO1_2 = 0x90; 
	PIO1_3 = 0x90; 
	PIO1_4 = 0x90; 
	PIO1_5 = 0x90; 
	PIO1_6 = 0x90; 
	PIO1_7 = 0x90; 
	PIO1_8 = 0x90; 
	PIO1_9 = 0x90; 
	PIO1_10 = 0x90; 
	PIO1_11 = 0x90; 
	PIO1_12 = 0x90; 
	PIO1_13 = 0x90; 
	PIO1_14 = 0x90; 
	PIO1_15 = 0x90; 
	PIO1_16 = 0x90; 
	PIO1_17 = 0x90; 
	PIO1_18 = 0x90; 
	PIO1_19 = 0x90; 
	PIO1_20 = 0x90; 
	PIO1_21 = 0x90; 
	PIO1_22 = 0x90; 
	PIO1_23 = 0x90; 
	PIO1_24 = 0x90; 
	PIO1_25 = 0x90; 
	PIO1_26 = 0x90; 
	PIO1_27 = 0x90; 
	PIO1_28 = 0x90; 
	PIO1_29 = 0x90; 
	PIO1_30 = 0x90; 
	PIO1_31 = 0x90; 
	PIO2_0 = 0x90; 
	PIO2_1 = 0x90; 
	PIO2_2 = 0x90; 
	PIO2_3 = 0x90; 
	PIO2_4 = 0x90; 
	PIO2_5 = 0x90; 
	PIO2_6 = 0x90; 
	PIO2_7 = 0x90; 
	PIO2_8 = 0x90; 
	PIO2_9 = 0x90; 
	PIO2_10 = 0x90; 
	PIO2_11 = 0x90; 
	PIO2_12 = 0x90; 
	PIO2_13 = 0x90; 
	PIO2_14 = 0x90; 
	PIO2_15 = 0x90; 
	PIO2_16 = 0x90; 
	PIO2_17 = 0x90; 
	PIO2_18 = 0x90; 
	PIO2_19 = 0x90; 
	PIO2_20 = 0x90; 
	PIO2_21 = 0x90; 
	PIO2_22 = 0x90; 
	PIO2_23 = 0x90; 

/* * /
	for( u32 i=0; i<sizeof(map)/sizeof(map[0]); ++i ){
	    std::cout << map[i].name
		      << " "
		      << std::hex
		      << (layout.base+(i*4))
		      << std::endl;
	}
	/* */
    }
    
}

