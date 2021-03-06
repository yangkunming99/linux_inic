#ifndef __SDIO_IO_H__
#define __SDIO_IO_H__
/***************************************************/
#include "basic_types.h"
#include "drv_types.h"
#include "rtw_io.h"
u8 sdio_read8(PADAPTER padapter, u32 addr);
u16 sdio_read16(PADAPTER padapter, u32 addr);
u32 sdio_read32(PADAPTER padapter, u32 addr);
u32 sdio_read_port(PADAPTER padapter, u32 addr, u32 cnt, u8 * mem);
u32 sdio_write_port(PADAPTER padapter, u32 addr, u32 cnt, u8 *mem);
s32 _sdio_local_read(PADAPTER padapter, u32 addr, u32 cnt, u8 *pbuf);
s32 sdio_local_read(PADAPTER padapter, u32 addr, u32 cnt, u8 *pbuf);
s32 _sdio_local_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pbuf);
s32 sdio_local_write(PADAPTER padapter, u32 addr, u32 cnt, u8 *pbuf);
extern u8 SdioLocalCmd52Read1Byte(PADAPTER padapter, u32 addr);
extern void SdioLocalCmd52Write1Byte(PADAPTER padapter, u32 addr, u8 v);
void sd_int_hal(PADAPTER padapter);
void sdio_set_intf_ops(PADAPTER padapter,struct _io_ops *pops);
void InitInterrupt8195ASdio(PADAPTER padapter);
void EnableInterrupt8195ASdio(PADAPTER padapter);
void DisableInterrupt8195ASdio(PADAPTER padapter);
/**************************************************/
#endif
