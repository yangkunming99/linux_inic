#ifndef __SDIO_OPS_LINUX_H__
#define __SDIO_OPS_LINUX_H__
#include "basic_types.h"
#include <linux/mmc/sdio_func.h>
#include "drv_types.h"
inline void rtw_sdio_set_irq_thd(struct dvobj_priv *dvobj, _thread_hdl_ thd_hdl);
s32 _sd_cmd52_read(struct sdio_func *func, u32 addr, u32 cnt, u8 *pdata);
s32 sd_cmd52_read(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata);
s32 _sd_cmd52_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata);
s32 sd_cmd52_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pdata);
u8 sd_read8(PADAPTER padapter, u32 addr, s32 *err);
u8 sd_f0_read8(PADAPTER padapter,u32 addr, s32 *err);
u16 sd_read16(PADAPTER padapter, u32 addr, s32 *err);
u32 sd_read32(PADAPTER padapter, u32 addr, s32 *err);
void sd_write8(PADAPTER padapter, u32 addr, u8 v, s32 *err);
void sd_write16(PADAPTER padapter, u32 addr, u16 v, s32 *err);
void sd_write32(PADAPTER padapter, u32 addr, u32 v, s32 *err);
s32 _sd_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata);
s32 sd_read(PADAPTER padapter, u32 addr, u32 cnt, void *pdata);
s32 _sd_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata);
s32 sd_write(PADAPTER padapter, u32 addr, u32 cnt, void *pdata);

#endif
