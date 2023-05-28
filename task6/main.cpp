#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
 
 
using namespace std;
 
int main()
{	
	//得到套接字描述符
	int sockfd;		

    /* code */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	/*
		AF_INET 参数指定了使用 IPv4 地址族。
		SOCK_DGRAM 参数指定了套接字的类型为数据报套接字（UDP）。
		最后一个参数为 0，表示选择默认的协议（由操作系统选择合适的协议）。
	*/
	struct ifconf ifc;
	caddr_t buf;
	int len = 100;
	
	//初始化ifconf结构
	ifc.ifc_len = 1024;
	if ((buf = (caddr_t)malloc(1024)) == NULL)
	{
		cout << "malloc error" << endl;
		exit(-1);
	}
	ifc.ifc_buf = buf; 
	
	//获取所有接口信息
	
    /* code */
	ioctl(sockfd, SIOCGIFCONF, &ifc);
	//通过调用 ioctl(sockfd, SIOCGIFCONF, &ifc)，将会填充 ifc 结构体中的数据，包括每个网络接口的名称、地址和其他属性。
	
	//遍历每一个ifreq结构
	struct ifreq *ifr;
	struct ifreq ifrcopy;
	ifr = (struct ifreq*)buf;
	for(int i = (ifc.ifc_len/sizeof(struct ifreq)); i>0; i--)
	{
		//接口名
		cout << "interface name: "<< ifr->ifr_name << endl;
		//ipv4地址
		cout << "inet addr: " 
			 << inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr)
			 << endl;
		
		//获取广播地址
		ifrcopy = *ifr;

		/* code */

		cout << "broad addr: "
			 << inet_ntoa(((struct sockaddr_in*)&(ifrcopy.ifr_addr))->sin_addr)
			 << endl;
		//获取mtu
		ifrcopy = *ifr;
		
        /* code */

		cout << "mtu: " << ifrcopy.ifr_mtu << endl;
		cout << endl;
		ifr++;
	}
	
	return 0;
}