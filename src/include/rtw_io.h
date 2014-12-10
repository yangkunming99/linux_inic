#ifndef _RTW_IO_H_
#define _RTW_IO_H_
#include "autoconf.h"
#include "drv_types.h"
struct _io_ops
{
	u8 (*_read8)(PADAPTER padapter, u32 addr);
	u16 (*_read16)(PADAPTER padapter, u32 addr);
	u32 (*_read32)(PADAPTER padapter, u32 addr);

	int (*_write8)(PADAPTER padapter, u32 addr, u8 val);
	int (*_write16)(PADAPTER padapter, u32 addr, u16 val);
	int (*_write32)(PADAPTER padapter, u32 addr, u32 val);
	int (*_writeN)(PADAPTER padapter, u32 addr, u32 length, u8 *pdata);

	void (*_read_mem)(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);
	void (*_write_mem)(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);

	u32 (*_read_port)(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);
	u32 (*_write_port)(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);

#ifdef CONFIG_SDIO_HCI
	u8 (*_sd_f0_read8)(PADAPTER padapter, u32 addr);
#endif
};


#define SD_IO_TRY_CNT (8)
#define MAX_CONTINUAL_IO_ERR SD_IO_TRY_CNT

int rtw_inc_and_chk_continual_io_error(struct dvobj_priv *dvobj);
void rtw_reset_continual_io_error(struct dvobj_priv *dvobj);
u32 rtw_write_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *pmem);

#endif
