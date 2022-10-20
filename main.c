#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <tools.h>
#include "ftp_client.h"

struct cmd_t
{
	const char* cmd;
	void (*cmdfp)();
};

void login_ftp(FTPClient* ftp)
{
	char arg[256];
	printf("Name:");
	get_str(arg,sizeof(arg));
	user_ftp(ftp,arg);
	printf("%s",ftp->rbuf);

	printf("Passwd:");
	get_passwd(arg,sizeof(arg),false);
	pass_ftp(ftp,arg);
	printf("%s",ftp->rbuf);
}

void undef(FTPClient* ftp,char* arg)
{
	printf("%s 该指令未定义!\n",arg);
}


static struct cmd_t cmds[] = {
	{"pwd",pwd_ftp},
	{"ls",ls_ftp},
	{"cd",cd_ftp},
	{"del",dele_ftp},
	{"mkdir",mkdir_ftp},
	{"rmdir",rmdir_ftp},
	{"get",get_ftp},
	{"put",put_ftp},
	{"bye",bye_ftp},
	{"user",login_ftp},
	{"",undef}
};

void sigint(int signum)
{
	exit(EXIT_SUCCESS);
}

void on_exit_fp(int status,void* arg)
{
	bye_ftp(arg);
}

int main(int argc,const char* argv[])
{
	FTPClient* ftp = create_ftp();
	
	signal(SIGINT,sigint);
	on_exit(on_exit_fp,ftp);
	
	if(2 == argc)
		connect_ftp(ftp,argv[1],21);
	else if(3 == argc)
		connect_ftp(ftp,argv[1],atoi(argv[2]));
	else
	{
		puts("Use:./ftp <ip> <port>");
		return EXIT_SUCCESS;
	}
	
	if(220 != ftp->code)
	{
		printf("%s",ftp->rbuf);
		return EXIT_FAILURE;
	}

	printf("Connected to %s.\n%s",argv[1],ftp->rbuf);
	login_ftp(ftp);

	char cmd[256] = {};
	char cmd_cnt = ARR_LEN(cmds);
	for(;;)
	{
		printf("ftp>");
		get_str(cmd,sizeof(cmd));
		if(!ftp->islogin && strncmp(cmd,"user",4) && strncmp(cmd,"bye",3))
		{
			printf("No control connection for command！\n");
			continue;
		}	

		for(int i=0; i<cmd_cnt; i++)
		{
			int len = strlen(cmds[i].cmd);
			if(0 == strncmp(cmd,cmds[i].cmd,len))
			{
				cmds[i].cmdfp(ftp,cmd+len+1);
				printf("%s",ftp->rbuf);
				break;
			}
		}	
	}
	return 0;
}
