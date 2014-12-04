#ifndef __8195_DESC_H__
#define __8195_DESC_H__
#if 1
typedef struct _TX_DESC{
	// u4Byte 0
	u32	txpktsize:16;
	u32	offset:8;    		// store the sizeof(SDIO_TX_DESC)
	u32	bus_agg_num:8;		// the bus aggregation number
	
	// u4Byte 1
	u32	rsvd1;
	
	// u4Byte 2
	u32	rsvd2;
	
	// u4Byte 3
	u32	rsvd3;
	
	// u4Byte 4
	u32	rsvd4;

	// u4Byte 5
	u32	rsvd5;
} TX_DESC, *PTX_DESC;
#else
typedef struct _TX_DESC{
	// DWORD 0
	unsigned int	txpktsize:16;
	unsigned int	offset:8;
	unsigned int	bmc:1;
	unsigned int	htc:1;
	unsigned int	ls:1;
	unsigned int	fs:1;
	unsigned int	linip:1;
	unsigned int	noacm:1;
	unsigned int	gf:1;            //Green Field enable
	unsigned int	own:1;           //When set, this bit indicates that the descriptor is owned by NIC
	
	// DWORD 1
	unsigned int	macid:5;			
	unsigned int	agg_en:1;
	unsigned int	bk:1;
	unsigned int	rdg_en:1;

	unsigned int	qsel:5;
	unsigned int    rdg_nav_ext:1;
	unsigned int	lsig_txop_en:1;	
	unsigned int	pifs:1;

	unsigned int	rate_id:4;
	unsigned int	navusehdr:1;
	unsigned int	en_desc_id:1;
	unsigned int	sectype:2;

	unsigned int	rsvd2:2;
	unsigned int	pkt_offset:5;
	unsigned int	rsvd3:1;
	
	// DWORD 2
	unsigned int    rts_rc:6;
	unsigned int    data_rc:6;
	unsigned int    rsvd8:2;	
	unsigned int	bar_rty_th:2;

	unsigned int    rsvd4:1;
	unsigned int	morefrag:1;
	unsigned int    raw:1;
	unsigned int    ccx:1;
	unsigned int    ampdu_density:3;
	unsigned int    rsvd5:1;
	
	unsigned int    antsel_a:1;
	unsigned int    antsel_b:1;
	unsigned int    tx_ant_cck:2;
	unsigned int	tx_antl:2;
	unsigned int    tx_antht:2;
	
	// DWORD 3
	unsigned int	nextheadpage:8;
	unsigned int	tailpage:8;
	unsigned int	seq:12;
	unsigned int	pkt_id:4;
	
	// DWORD 4
	unsigned int	rtsrate:5;
	unsigned int    ap_dcfe:1;
	unsigned int    qos: 1;
	unsigned int    hwseq_en:1;
	
	unsigned int	userate:1;
	unsigned int	disrtsfb:1;
	unsigned int	disdatafb:1;
	unsigned int	cts2self:1;
	unsigned int	rtsen:1;
	unsigned int	hw_rts_en:1;
	unsigned int	port_toggle:1;
	unsigned int    rsvd6:1;
	
	unsigned int    rsvd7:2;
	unsigned int	wait_dcts:1;
	unsigned int	cts2ap_en:1;
	unsigned int	data_txsc:2;
	unsigned int	data_stbc:2;
	
	unsigned int	data_short:1;
	unsigned int	databw:1;
	unsigned int	rts_short:1;
	unsigned int	rtsbw:1;
	unsigned int	rts_sc:2;
	unsigned int	vcs_stbc:2;

	// DWORD 5
	unsigned int	datarate:8;

	unsigned int	data_ratefb_lmt:5;
	unsigned int	rts_ratefb_lmt:4;
	unsigned int	rty_en:1;
	unsigned int	data_rt_lmt:6;

	unsigned int	usb_txagg_num:8;
	
	// DWORD 6
	unsigned int	txagc_a:5;
	unsigned int	txagc_b:5;
	unsigned int	use_max_len:1;
	unsigned int	max_agg_num:5;
	unsigned int	mcsg1_max_len:4;
	unsigned int	mcsg2_max_len:4;
	unsigned int	mcsg3_max_len:4;
	unsigned int	mcsg7_max_len:4;

	// DWORD 7
unsigned int	txbuffsize:16;		//PCIe
unsigned int	mcsg4_max_len:4;
unsigned int	mcsg5_max_len:4;
unsigned int	mcsg6_max_len:4;
unsigned int	mcsg15_max_len:4;
}TX_DESC, *PTX_DESC;
#endif
typedef struct _TX_DESC TXDESC_8195A, *PTXDESC_8195A;
#define	SIZE_TX_DESC_8195a	(sizeof(TX_DESC))

#if 1
typedef struct _RX_DESC{
	// u4Byte 0
	u32	pkt_len:16;
	u32	offset:8;    	
	u32	rsvd0:6;        //
	u32	icverr:1;
	u32	crc32:1;

	// u4Byte 1
	u32	rsvd1;
	
	// u4Byte 2
	u32	rsvd2;
	
	// u4Byte 3
	u32	rsvd3;
	
	// u4Byte 4
	u32	rsvd4;

	// u4Byte 5
	u32	rsvd5;
} RX_DESC, *PRX_DESC;
#else
typedef struct _RX_DESC{
	// DWORD 0
	unsigned int	pkt_len:14;
	unsigned int	crc32:1;
	unsigned int	icverr:1;
	unsigned int	drv_info_size:4;
	unsigned int	security:3;
	unsigned int	qos:1;
	unsigned int	shift:2;
	unsigned int	physt:1;
	unsigned int	swdec:1;
	unsigned int	ls:1;
	unsigned int	fs:1;
	unsigned int	eor:1;
	unsigned int	own:1;

	// DWORD 1
	unsigned int	macid:5;			
	unsigned int	tid:4;
	unsigned int	hwrsvd:4;
	unsigned int	amsdu:1;
	unsigned int	paggr:1;
	unsigned int	faggr:1;
	unsigned int	a1_fit:4;
	unsigned int	a2_fit:4;
	unsigned int	pam:1;
	unsigned int	pwr:1;
	unsigned int	md:1;
	unsigned int	mf:1;
	unsigned int	type:2;
	unsigned int	mc:1;
	unsigned int	bc:1;
	
	// DWORD 2
	unsigned int	seq:12;
	unsigned int	frag:4;
	unsigned int	usb_agg_pkt_num:8;
	unsigned int	rsvd1:6;
	unsigned int	next_ind:1;
	unsigned int	rsvd2:1;
	
	// DWORD 3
	unsigned int	rxmcs:6;			
	unsigned int	rxht:1;
	unsigned int	gf:1;
	unsigned int	splcp:1;
	unsigned int	bw:1;
	unsigned int	htc:1;
	unsigned int	eosp:1;
	unsigned int	bssid_fit:2;
	unsigned int	hwpc_err:1;
	unsigned int	hwpc_ind:1;
	unsigned int	iv1:16;
	
	// DWORD 4
	unsigned int	iv;

	// DWORD 5
	unsigned int	tsfl;
} RX_DESC, *PRX_DESC;
#endif
#define	SIZE_RX_DESC_8195a	(sizeof(RX_DESC))
typedef struct _RX_DESC RXDESC_8195A, *PRXDESC_8195A;

#endif
