

void rtl8195a_set_hal_ops(PADAPTER padapter)
{
	struct hal_ops *pHalFunc = &padapter->HalFunc;
	if(padapter->interface_type == RTW_SDIO){
		pHalFunc->hal_xmit = &rtl8195as_hal_xmit;
		pHalFunc->hal_mgnt_xmit = &rtl8195as_hal_mgnt_xmit;
		pHalFunc->xmit_thread_handler = &rtl8195as_hal_xmit_handler;
	}else if(padapter->interface_type == RTW_USB)
	{
		pHalFunc->hal_xmit = &rtl8195au_hal_xmit;
		pHalFunc->hal_mgnt_xmit = &rtl8195au_hal_mgnt_xmit;
		pHalFunc->xmit_thread_handler = &rtl8195au_hal_xmit_handler;
	}else
	{
		pHalFunc->hal_xmit = NULL;
		pHalFunc->hal_mgnt_xmit = NULL;
		pHalFunc->xmit_thread_handler = NULL;
	}
}