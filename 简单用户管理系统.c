#include <stdio.h>
#include <getch.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

//主要变量声明
//-----------------------------------------------------------------

//用户信息结构体
typedef struct user{
	char name[20];
	char passward[7];
	int lock;
	}ur;

//全局变量
size_t cnt = 0;
ur users[100];

//辅助函数
//-----------------------------------------------------------------

//显示信息防止清屏不显示函数
void show_msg(const char* msg,float sec)
{
	printf("\033[10;32m%s\033[00m\n",msg);
	usleep(sec*1000000);
}

//获取字符函数
char* get_str(char* str,size_t size)
{
	assert(NULL!=str && size>1);
	//计算fgets读取了多少个字符
	size_t len = strlen(fgets(str,size,stdin));
	//如果最后一个字符是'\n',则把它改为'\0'
	if('\n' == str[len -1])
	{	
		str[len-1] = '\0';
	}
	else
	{	//如果最后一个字符不是'\n',说明缓冲区中有垃圾数据，则需要清理输入缓冲区
		while('\n' != getchar());
	}
	return str;
}

//查重函数
int check_repeat(char* str)
{
	for(int i=0; i<cnt; i++)
	{
		if(0 == strcmp(users[i].name,str))
			return i;
	}
	return -1;
}

//显示菜单界面函数
void menu()
{
	system("clear");
	printf(" 1、注册 2、登录 3、显示所有用户 4、退出系统\n");
}
//获取并隐藏密码函数
char* get_passwd(char* passwd,size_t size)
{
	int i = 0;
	while(i < size-1)
	{
		passwd[i] = getch();
		//读取到退格键
		if(127 == passwd[i])
		{
			//说明数组中还有已输入的密码
			if(i>0)
			{
				//删除一位密码
				i--;
				//删除一个*号
			 	printf("\b \b");
			}
			continue;
		}
		//密码增加一位
		i++;
		//增加一个*号
		printf("*");
	}
	passwd[size - 1]= '\0';
	printf("\n");
	return passwd;
}

//确认操作函数
bool yes_or_no(void)
{
	printf("是否确认此操作(y/n)");
	for(;;)
	{
		char cmd = getch();
		if('y' == cmd || 'Y' == cmd || 'n' == cmd|| 'N' == cmd)
		{
			printf("%c\n",cmd);
			return 'y' == cmd || 'Y' == cmd;
		}
	}
}

//获取命令函数
int get_cmd(char start,char end)
{
	//确保end>start
	assert(start <= end);
	puts("----------------------------------");
	printf("请输入相应的数字选择需要的服务\n");
	for(;;)
	{
		int cmd = getch();
		if(cmd >= start && cmd <= end)
		{
			printf("%c\n",cmd);
			return cmd;
		}
		else
		{
			printf("输入错误,请重新输入\n");
		}
	}
}
//任意键继续函数
void anykey_continue(void)
{	
	puts("按任意键继续...");
	getch();
}

//主要函数
//-----------------------------------------------------------------

//注册函数
void usr_register(void)
{
	puts("----------------------------------");
	printf("用户注册界面\n");
	if(cnt > 99)
	{
		show_msg("已到达注册上限\n",1);
		return;
	}
	//输入用户名以及查重
	printf("请输入小于20个字的用户名\n");
	get_str(users[cnt].name,sizeof(users[cnt].name));
	
	if(0 <= check_repeat(users[cnt].name))
	{
		show_msg("该用户已经存在,不能添加\n",1);
		return;
	}
	//输入密码以及确认密码
	
		printf("请输入6位密码\n");
		get_passwd(users[cnt].passward,sizeof(users[cnt].passward));
		char again[7];
	for(;;)
	{
		printf("请确认密码\n");
		get_passwd(again,7);
		if(0 == strcmp(users[cnt].passward,again))
		{
			break;
		}
			printf("输入错误\n");
	}
	users[cnt].lock = 0;
	cnt++;
	show_msg("注册成功!\n",1);
}

//用户登录函数
void user_login(void)
{
	puts("----------------------------------");
	printf("用户登录界面\n");
	//输入用户名以及检测用户名所在位置
	char username[20];
    int index = -1;
	printf("请输入用户名\n");
	get_str(username,20);
	index = check_repeat(username);
	if(index < 0)
	{
		show_msg("用户名错误或者不存在\n",1);
		return;
	}
	for(;;)
	{
		//检测账户是否锁定
		if(users[index].lock > 2)
		{
			show_msg("账号已被锁定\n",1);
			return;
		}
		//检测密码并且三次密码错误后锁定账户
		char passward[7];
		printf("请输入密码\n");
		get_passwd(passward,7);
		if(0 != strcmp(passward,users[index].passward))
		{
			//锁定标记+1并提示剩余次数
			switch(++users[index].lock)
			{
				case 1:show_msg("密码错误，还剩2次机会",1);break;
				case 2:show_msg("密码错误，还剩1次机会",1);break;
				case 3:show_msg("密码错误,账户已被锁定",1);return;
			}
		}
		else
		{
			break;
		}
	}
	users[index].lock = 0;
	show_msg("登录成功！",1);
	user_host(index);
	return;
	
}
//用户界面函数
void user_host(int index)
{
	system("clear");
	puts("----------------------------------");
	puts("用户界面");
	puts("1、注销用户 2、修改密码 3、修改用户信息 4、退出登录");
	puts("请输入对应的数字选择功能");
	switch(get_cmd('1','4'))
	{
		case'1':logout(index);break;
		case'2':psw_change(index);break;
		case'3':name_change(index);break;
		case'4':show_msg("成功退出登录",1);break;
	}
	return;
}

//用户界面子菜单
//-----------------------------------------------------------------

//1、注销用户
void logout(int index)
{
	if(yes_or_no())
	{
		//把最后一个用户赋值给要删除的用户位置
		users[index] = users[cnt-1];
		//用户数量-1
		cnt--;
		//还原用户的登录状态
		index = -1;
		show_msg("您的账户已注销",1);
	}
	else
	{
		show_msg("已取消注销操作",1);
		return;
	}
}
//2、修改密码
void psw_change(int index)
{
	char usedpassward[7];
	char passward[7];
	char repassward[7];
	//检测输入的旧密码是否正确
	printf("请输入旧的密码\n");
	get_passwd(usedpassward,7);
	if(0 != strcmp(usedpassward,users[index].passward))
	{
		show_msg("密码错误!",1);
		return;
	}
	printf("请输入新的密码\n");
	get_passwd(passward,7);
	//二次确认密码
	for(;;)
	{
		printf("请确认密码\n");
		get_passwd(repassward,7);
		if(0 == strcmp(passward,repassward))
		{
			break;
		}
			show_msg("密码不一致",1);
	}
	//把新密码覆盖旧的密码
	strcpy(users[index].passward,passward);
	show_msg("密码修改成功！",1);
	return;
}
//3、修改用户信息
void name_change(int index)
{
	printf("请输入新的用户名\n");
	get_str(users[index].name,sizeof(users[index].name));
	show_msg("用户名修改成功！",1);
	return;
}
//-----------------------------------------------------------------

//显示所有用户信息函数
void show_users(void)
{
	for(int i=0; i < cnt; i++)
	{
		printf("用户名：%s\n密码：%s\n密码输入错误次数:%d\n",users[i].name,users[i].passward,users[i].lock);
	}
	anykey_continue();
}
//主程序
int main(int argc,const char* argv[])
{
	for(;;)
	{
		//显示菜单
		menu();

		//获取命令并调用相应的函数
		switch(get_cmd('1','4'))
		{
			case '1':usr_register();break;
			case '2':user_login();break;
			case '3':show_users();break;
			case '4':return 0;break;
		}
	}

	return 0;
}
