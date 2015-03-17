#include <stdio.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <linux/wireless.h>

#define RTL_IOCTL_ATCMD				(SIOCDEVPRIVATE+1)
#define ATCMD_BUFLEN 	64
unsigned char dev[IFNAMSIZ] = {0};
static void atcmd_ctl(char *pbuf, int len)
{
	int skfd, err;
	struct iwreq iwrq;
	if(len <= 1)
		return;
	/* Create a channel to the NET kernel. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}
	strcpy(iwrq.ifr_ifrn.ifrn_name, dev);
	iwrq.u.data.pointer = pbuf;
	iwrq.u.data.length = len;
	err = ioctl(skfd, RTL_IOCTL_ATCMD, &iwrq);
	if(err != 0)
	{		
		printf("err(%d): %s failed!\n", err, __FUNCTION__);		
	}
	close(skfd);
}

int main(int argc, char **argv)
{
	char buf[ATCMD_BUFLEN];
	if(argc < 2){
		printf("err: net device name must be given!\n");
		return -1;
	}
	strncpy(dev, argv[1], IFNAMSIZ-1);
	do{
		memset(buf, 0, sizeof(buf));
		printf("\n\rAT: ");
		fgets((char *)buf, ATCMD_BUFLEN, stdin);
		buf[strlen(buf)-1]='\0';//or here is the carrier return '\r'
		if(strcmp((const char *)buf, (const char *)("exit")) == 0) {
			break;
		}
		atcmd_ctl((char *)buf, strlen(buf));
	}while(1);
	return 0;
}
