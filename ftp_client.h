#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#define BUF_SIZE 	4096


typedef struct FTPClient
{
	int cfd;		// 命令通道socket对象描述符
	int dfd;		// 数据通道socket对象描述符
	bool islogin;	// 是否登录成功
	bool isget;		// 是否处于下载状态
	bool isput;		// 是否处于上传状态 
	int code;		// FTP状态代码
	char* sbuf;		// 用于发送命令的缓冲区
	char* rbuf;		// 用于接收结果的缓冲区
	char file_name[PATH_MAX]; 	// 将要下载的文件名  
	uint32_t file_size;				// 文件的大小
	char file_mdtm[15]; 		// 文件的最后修改时间
}FTPClient;

FTPClient* create_ftp(void);
void connect_ftp(FTPClient* ftp,const char* ip,short port);
void user_ftp(FTPClient* ftp,char* arg);
void pass_ftp(FTPClient* ftp,char* arg);
void pwd_ftp(FTPClient* ftp);
void ls_ftp(FTPClient* ftp);
void cd_ftp(FTPClient* ftp,char* arg);
void dele_ftp(FTPClient* ftp,char* arg);
void mkdir_ftp(FTPClient* ftp,char* arg);
void rmdir_ftp(FTPClient* ftp,char* arg);
void get_ftp(FTPClient* ftp,char* arg);
void put_ftp(FTPClient* ftp,char* arg);
void bye_ftp(FTPClient* ftp);


#endif//FTP_CLIENT_H
