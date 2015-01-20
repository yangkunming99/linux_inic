#ifndef _8195_SDIO_REG_H
#define _8195_SDIO_REG_H
#include "basic_types.h"


//For 8195A SDIO
#define SDIO_LOCAL_DEVICE_ID				0
#define WLAN_TX_FIFO_DEVICE_ID			4
#define WLAN_RX_FIFO_DEVICE_ID			7
#define UNDEFINED_DEVICE_ID				(-1)

#define SDIO_MAX_TX_QUEUE					3 //HIQ, MIQ, LOQ
#define SDIO_TX_FREE_PG_QUEUE				4 

#define SDIO_LOCAL_MSK						0xFFF
#define WLAN_TX_FIFO_MSK					0x1FFF
#define WLAN_RX_FIFO_MSK 					0x3


#define SDIO_LOCAL_OFFSET					0x10250000
#define TX_FIFO_OFFSET						0x10310000
#define RX_FIFO_OFFSET						0x10340000


//SDIO Local registers
#define SDIO_REG_TX_CTRL						0x00 // 1byte
#define SDIO_REG_STATIS_RECOVERY_TIMOUT	0x02 // 2bytes
#define SDIO_REG_32K_TRANS_IDLE_TIME		0x04 // 2bytes
#define SDIO_REG_HIMR							0x14 // 4bytes
#define SDIO_REG_HISR							0x18 // 4bytes
#define SDIO_REG_RX0_REQ_LEN					0x1c // 4bytes
#define SDIO_REG_FREE_TXBD_NUM				0x20 // 2bytes
#define SDIO_REG_TX_SEQNUM 					0x24 // 1byte, not used
#define SDIO_REG_HCPWM						0x38 // 1byte, host domain, sync from CCPWM
#define SDIO_REG_HCPWM2						0x3a // 2bytes, sync from CCPWM2
#define SDIO_REG_H2C_MSG						0x60 // 4bytes, host domain, sync from CPU_H2C_MSG
#define SDIO_REG_C2H_MSG						0x64 // 4bytes, sync from CPU_C2H_MSG
#define SDIO_REG_H2C_MSG_EXT					0x68 // 4bytes, sync from CPU_H2C_MSG_EXT
#define SDIO_REG_C2H_MSG_EXT					0x6c // 4bytes, sync from CPU_C2H_MSG_EXT
#define SDIO_REG_HRPWM						0x80 // 1byte, driver to FW, host domain, sync to CRPWM
#define SDIO_REG_HRPWM2						0x82 // 2bytes, driver to FW, host domain, sync to CRPWM2

// SDIO Host Interrupt Service Routine
#define SDIO_HISR_RX_REQUEST				BIT0
#define SDIO_HISR_AVAL_INT				BIT1
#define SDIO_HISR_TXPKT_OVER_BUFF		BIT2
#define SDIO_HISR_TX_AGG_SIZE_MISMATCH	BIT3
//BIT4~16 not used
#define SDIO_HISR_C2H_MSG_INT			BIT17
#define SDIO_HISR_CPWM1					BIT18
#define SDIO_HISR_CPWM2					BIT19
#define SDIO_HISR_H2C_BUS_FAIL			BIT20
#define SDIO_HISR_TXBD_OVF				BIT21
#define SDIO_HISR_CPU_NOT_RDY			BIT22
//BIT23~31 not used

#define MASK_SDIO_HISR_CLEAR				(SDIO_HISR_TXPKT_OVER_BUFF|\
											SDIO_HISR_TX_AGG_SIZE_MISMATCH|\
											SDIO_HISR_C2H_MSG_INT|\
											SDIO_HISR_CPWM1|\
											SDIO_HISR_CPWM2|\
											SDIO_HISR_H2C_BUS_FAIL|\
											SDIO_HISR_TXBD_OVF|\
											SDIO_HISR_CPU_NOT_RDY)
											
// RTL8195A SDIO Host Interrupt Mask Register
#define SDIO_HIMR_RX_REQUEST_MSK		BIT0
#define SDIO_HIMR_AVAL_MSK				BIT1
#define SDIO_HIMR_TXPKT_SIZE_OVER_BUFF_MSK		BIT2
#define SDIO_HIMR_AGG_SIZE_MISMATCH_MSK		BIT3
//BIT4~16 not used
#define SDIO_HIMR_C2H_MSG_MSK					BIT17
#define SDIO_HIMR_CPWM1_MSK						BIT18
#define SDIO_HIMR_CPWM2_MSK						BIT19
#define SDIO_HIMR_H2C_BUS_FAIL_MSK				BIT20
#define SDIO_HIMR_TXBD_OVF_MSK					BIT21
#define SDIO_HIMR_CPU_NOT_RDY_MSK				BIT22
//BIT23~31 not used

#define SDIO_HIMR_DISABLED			0
#endif
