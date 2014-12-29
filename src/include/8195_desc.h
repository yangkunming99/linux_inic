#ifndef __8195_DESC_H__
#define __8195_DESC_H__

// define transmit packat type
#define PACKET_802_3	0X83
#define PACKET_802_11	0X81
#define H2C_CMD			0X11
#define MEM_READ		0X51
#define MEM_WRITE		0X53
#define MEM_SET			0X55
#define FM_FREETOGO		0X61

typedef struct _TX_DESC{
	// u4Byte 0
	u32	txpktsize:16;
	u32	offset:8;    		// store the sizeof(TX_DESC)
	u32	bus_agg_num:8;		// the bus aggregation number
	
	// u4Byte 1
	/***********************************************/
	/**************transmit packet type****************/
	/*	0x83:	802.3 packet						      */
	/*	0x81:	802.11 packet						      */
	/*	0x11:	H2C command					      */
	/*	0x51:	Memory Read						      */
	/*	0x53:	Memory Write						      */
	/*	0x55:	Memory Set						      */
	/*	0x61:	Jump to firmware start				      */
	u32	type:8;//packet type
	u32	rsvd0:24;
	
	// u4Byte 2
	u32	rsvd1;
	
	// u4Byte 3
	u32	rsvd2;
	
	// u4Byte 4
	u32	rsvd3;

	// u4Byte 5
	u32	rsvd4;
} TX_DESC, *PTX_DESC;
typedef struct _TX_DESC TXDESC_8195A, *PTXDESC_8195A;
#define	SIZE_TX_DESC_8195a	(sizeof(TX_DESC))


typedef struct _RX_DESC{
	// u4Byte 0
	u32	pkt_len:16;
	u32	offset:8;    	
	u32	rsvd0:6;        //
	u32	icverr:1;
	u32	crc32:1;

	// u4Byte 1
	/************************************************/
	/*****************receive packet type*********************/
	/*	0x82:	802.3 packet						      */
	/*	0x80:	802.11 packet						      */
	/*	0x10:	C2H command					      */
	/*	0x50:	Memory Read						      */
	/*	0x52:	Memory Write						      */
	/*	0x54:	Memory Set						      */
	/*	0x60:	Indicate the firmware is started		      */
	u32 type:8;
	u32	rsvd1:24;
	
	// u4Byte 2
	u32	rsvd2;
	
	// u4Byte 3
	u32	rsvd3;
	
	// u4Byte 4
	u32	rsvd4;

	// u4Byte 5
	u32	rsvd5;
} RX_DESC, *PRX_DESC;
#define	SIZE_RX_DESC_8195a	(sizeof(RX_DESC))
typedef struct _RX_DESC RXDESC_8195A, *PRXDESC_8195A;

#endif
