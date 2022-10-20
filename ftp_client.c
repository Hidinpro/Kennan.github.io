#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <utime.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "ftp_client.h"

static const char* get_file_mdtm(const char* path)
{
	static char mdtm[15];
	
	struct stat buf;
	if(stat(path,&buf))
	{
		return NULL;
	}
	
	struct tm* tm = localtime(&buf.st_mtim.tv_sec);	
	sprintf(mdtm,"%04d%02d%02d%02d%02d%02d",
		tm->tm_year+1900,
		tm->tm_mon+1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);
	
	return mdtm;
}

static void set_file_mdtm(const char* path,const char* mdtm)
{
	struct stat buf;
	if(stat(path,&buf))
	{
		return;
	}
	
	struct tm tm;
	sscanf(mdtm,"%4d%2d%2d%2d%2d%2d",
		&tm.tm_year,
		&tm.tm_mon,
		&tm.tm_mday,
		&tm.tm_hour,
		&tm.tm_min,
		&tm.tm_sec);
	
	/*
struct tm {
int tm_sec;  秒 – 取值区间为[0,59] 
int tm_min;  分 - 取值区间为[0,59] 
int tm_hour;  时 - 取值区间为[0,23] 
int tm_mday;  一个月中的日期 - 取值区间为[1,31] 
int tm_mon;  月份（从一月开始，0代表一月） - 取值区间为[0,11] 
int tm_year;  年份，其值等于实际年份减去1900 
int tm_wday;  星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 
int tm_yday;  从每年的1月1日开始的天数 – 取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 
int tm_isdst;  夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。

	*/
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	
	struct utimbuf times;
	times.modtime = mktime(&tm);//mktime()用来将参数 tm 所指的tm结构数据转换成从公元1970年1月1日0时0分0 秒算起至今的UTC时间所经过的秒数。
	times.actime = buf.st_atim.tv_sec;
	
	utime(path,&times);
}

static uint32_t get_file_size(const char* path)
{
	int fd = open(path,O_RDONLY|O_APPEND);
	if(0 > fd)
	{
		return -1;
	}
	
	return lseek(fd,0,SEEK_END);
}

static void recv_result(FTPClient* ftp)
{
	int index = 0;
	do{
		if(0 > recv(ftp->cfd,ftp->rbuf+index,1,0))
		{
			sprintf(ftp->rbuf,"recv:%m\n");
			ftp->code = -1;
			return;
		}
	}while('\n' != ftp->rbuf[index++]);
	ftp->rbuf[index] = '\0';
	sscanf(ftp->rbuf,"%d",&ftp->code);
}

static void send_cmd(FTPClient* ftp)
{
	if(0 > send(ftp->cfd,ftp->sbuf,strlen(ftp->sbuf),0))
	{
		sprintf(ftp->rbuf,"send:%m\n");
		ftp->code = -1;
		return;
	}
	
	recv_result(ftp);
}

FTPClient* create_ftp(void)
{
	FTPClient* ftp = malloc(sizeof(FTPClient));
	ftp->sbuf = malloc(BUF_SIZE);
	ftp->rbuf = malloc(BUF_SIZE);
	
	ftp->cfd = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->cfd)
	{
		perror("socket");
		free(ftp);
		return NULL;
	}

	ftp->islogin = false;
	ftp->isget = false;
	ftp->isput = false;
	return ftp;
}

void connect_ftp(FTPClient* ftp,const char* ip,short port)
{
	struct sockaddr_in addr = {AF_INET};
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if(connect(ftp->cfd,(struct sockaddr*)&addr,sizeof(addr)))
	{
		sprintf(ftp->rbuf,"Connect:%m\n");
		return;
	}

	recv_result(ftp);
}

void user_ftp(FTPClient* ftp,char* arg)
{
	sprintf(ftp->sbuf,"USER %s\r\n",arg);
	send_cmd(ftp);
}

void pass_ftp(FTPClient* ftp,char* arg)
{
	sprintf(ftp->sbuf,"PASS %s\r\n",arg);
	send_cmd(ftp);
	
	ftp->islogin = (230 == ftp->code);
}

void pwd_ftp(FTPClient* ftp)
{
	sprintf(ftp->sbuf,"PWD\r\n");
	send_cmd(ftp);
}

void pasv_ftp(FTPClient* ftp)
{
	sprintf(ftp->sbuf,"PASV\r\n");
	send_cmd(ftp);

	if(227 != ftp->code)
		return;

	printf("%s",ftp->rbuf);

	ftp->dfd = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->dfd)
	{
		sprintf(ftp->rbuf,"socket:%m\n");
		ftp->code = -1;
		return;
	}

	struct sockaddr_in addr = {AF_INET};
	uint8_t* port = (uint8_t*)&addr.sin_port;
	uint8_t* ip = (uint8_t*)&addr.sin_addr.s_addr;
	sscanf(strchr(ftp->rbuf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
		ip,ip+1,ip+2,ip+3,port,port+1);
		
	if(connect(ftp->dfd,(struct sockaddr*)&addr,sizeof(addr)))
	{
		sprintf(ftp->rbuf,"Connect:%m\n");
		return;
	}
}

void read_or_write(int rfd,int wfd)
{
	char buf[BUF_SIZE];
	//while(write(wfd,buf,read(rfd,buf,BUF_SIZE)));
	
	int ret = 0;
	while((ret = read(rfd,buf,1)))
	{
		write(wfd,buf,ret);
		sleep(1);
		printf("*");
		fflush(stdout);
	}
}

void ls_ftp(FTPClient* ftp)
{
	pasv_ftp(ftp);
	
	sprintf(ftp->sbuf,"LIST -al\r\n");
	send_cmd(ftp);
	
	if(150 != ftp->code)
	{
		close(ftp->dfd);
		return;
	}
	
	read_or_write(ftp->dfd,STDOUT_FILENO);
	close(ftp->dfd);
	recv_result(ftp);
}

void cd_ftp(FTPClient* ftp,char* arg)
{
	sprintf(ftp->sbuf,"CWD %s\r\n",arg);
	send_cmd(ftp);	
}

void dele_ftp(FTPClient* ftp,char* arg)
{	
	sprintf(ftp->sbuf,"DELE %s\r\n",arg);
	send_cmd(ftp);		
}

void mkdir_ftp(FTPClient* ftp,char* arg)
{
	sprintf(ftp->sbuf,"MKD %s\r\n",arg);
	send_cmd(ftp);	
}

void rmdir_ftp(FTPClient* ftp,char* arg)
{
	sprintf(ftp->sbuf,"RMD %s\r\n",arg);
	send_cmd(ftp);	
}

void get_ftp(FTPClient* ftp,char* arg)
{	
	// 设置二进制文件传输模式
	sprintf(ftp->sbuf,"TYPE I\r\n");
	send_cmd(ftp);
	
	// 获取文件的字节数
	sprintf(ftp->sbuf,"SIZE %s\r\n",arg);
	send_cmd(ftp);
	if(213 != ftp->code)
	{
		return;
	}
	
	// 备份文件的字节数
	sscanf(ftp->rbuf,"%*d %u",&ftp->file_size);
	
	// 获取文件的最后修改时间
	sprintf(ftp->sbuf,"MDTM %s\r\n",arg);
	send_cmd(ftp);
	if(213 != ftp->code)
	{
		return;
	}
	
	// 备份文件的最后修改时间
	sscanf(ftp->rbuf,"%*d %s",ftp->file_mdtm);
	
	// 打开数据通道
	pasv_ftp(ftp);
	
	// 判断是否需要续传 字节数不同 本地 < 服务器 && 文件的最后修改时间相同
	int wfd = -1;
	if(get_file_size(arg) < ftp->file_size && 0 == strcmp(ftp->file_mdtm,get_file_mdtm(arg)))
	{
		sprintf(ftp->sbuf,"REST %u\r\n",get_file_size(arg));
		send_cmd(ftp);
		wfd = open(arg,O_WRONLY|O_APPEND);
		printf("开启断点续传！\n");
	}
	else
	{
		wfd = open(arg,O_WRONLY|O_CREAT,0644);
	}
	
	if(0 > wfd)
	{
		sprintf(ftp->rbuf,"%s open:%m\n",arg);
		close(ftp->dfd);
		return;
	}
	
	// 备份文件名
	strcpy(ftp->file_name,arg);
	
	// 标记正在下载
	ftp->isget = true;
	
	// 告诉服务端开始传输文件内容
	sprintf(ftp->sbuf,"RETR %s\r\n",arg);
	send_cmd(ftp);	

	if(150 != ftp->code)
	{
		close(ftp->dfd);
		return;
	}
	
	// 从服务端读取数据并写入文件
	read_or_write(ftp->dfd,wfd);
	
	// 关闭数据通道、文件
	close(ftp->dfd);
	close(wfd);
	
	// 设置文件的最后修改时间 
	set_file_mdtm(arg,ftp->file_mdtm);
	
	// 取消下载标记
	ftp->isget = false;
	
	recv_result(ftp);
}

void put_ftp(FTPClient* ftp,char* arg)
{
	int rfd = open(arg,O_RDONLY);
	if(0 > rfd)
	{
		sprintf(ftp->rbuf,"%s open:%m\n",arg);
		return;
	} 

	pasv_ftp(ftp);
	
	sprintf(ftp->sbuf,"STOR %s\r\n",arg);
	send_cmd(ftp);

	if(150 != ftp->code)
	{
		close(ftp->dfd);
		close(rfd);
		return;
	}

	read_or_write(rfd,ftp->dfd);
	close(ftp->dfd);
	recv_result(ftp);
}

void bye_ftp(FTPClient* ftp)
{
	// 判断是否有正在下载的文件
	if(ftp->isget)
	{
		// 设置该文件的最后修改时间
		set_file_mdtm(ftp->file_name,ftp->file_mdtm);
		printf("异常退出，设置文件的最后修改时间！\n");
	}
	
	close(ftp->cfd);
	close(ftp->dfd);
	free(ftp->sbuf);
	free(ftp->rbuf);
	free(ftp);
	printf("\nGoodbye.\n");
	_exit(EXIT_SUCCESS);
}
