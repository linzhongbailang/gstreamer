#ifdef __ONLINE_DEBUG_ENABLE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ANDROID
#include <errno.h>
#else
#include <error.h>
#endif

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <linux/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/prctl.h>


#include "../inc/debug_online_common.h"   

#define LOG_ERROR printf
#define LOG_INFO  printf

typedef struct dbg_online_InternCbCmdParam{
	unsigned int used;
	char Name[DEBUG_ONLINE_CMD_NAME_LENGTH];
	Debug_Online_CbCmdFunc CmdCbFunc;
	HelpFunc help;
	struct dbg_online_InternCbCmdParam *next;
}Debug_Online_InternCbCmdParam_t;

static pthread_mutex_t DBG_Online_CmdTableLock = PTHREAD_MUTEX_INITIALIZER;

static Debug_Online_InternCbCmdParam_t DBG_Online_RegisterCmdTable[DEBUG_ONLINE_CMD_COUNT];
static int cur_sock=-1;


void Debug_Online_Server_InitCmdList(void)
{
	int i = 0;

	pthread_mutex_lock(&DBG_Online_CmdTableLock);
	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		
		memset(DBG_Online_RegisterCmdTable[i].Name, 0, DEBUG_ONLINE_CMD_NAME_LENGTH);
		DBG_Online_RegisterCmdTable[i].CmdCbFunc = NULL;
		DBG_Online_RegisterCmdTable[i].help = NULL;
		DBG_Online_RegisterCmdTable[i].used = 0;
		DBG_Online_RegisterCmdTable[i].next = NULL;
		
	}
	pthread_mutex_unlock(&DBG_Online_CmdTableLock);
}

int Debug_Online_Server_RegistCmd(const Debug_Online_CbCmdParam_t *in_cmd)
{
	int i;
	
	if( (in_cmd == NULL) || (in_cmd->CmdCbFunc == NULL) || \
		 (in_cmd->help == NULL) || (!strlen(in_cmd->Name)) )
	{
		LOG_ERROR(" Invalid Arg!!\n");
		return -1;
	}

	if( !strcasecmp(in_cmd->Name,DEBUG_ONLINE_STRING_QUIT) || \
		 !strcasecmp(in_cmd->Name,DEBUG_ONLINE_STRING_HELP) )
	{
		LOG_ERROR("%s and %s is reserved name in DEBUGONLINE\n",\
					DEBUG_ONLINE_STRING_QUIT,\
					DEBUG_ONLINE_STRING_HELP);
		return -1;
	}
		
	pthread_mutex_lock(&DBG_Online_CmdTableLock);
	
	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		if( (DBG_Online_RegisterCmdTable[i].used == 1)  &&
			 (strcasecmp(in_cmd->Name, DBG_Online_RegisterCmdTable[i].Name) == 0) )
		{
			LOG_ERROR(" Cmd %s already exist!!\n", in_cmd->Name);
			pthread_mutex_unlock(&DBG_Online_CmdTableLock);
			return -1;
		}
	}

		
	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		if(DBG_Online_RegisterCmdTable[i].used == 0)
		{
			DBG_Online_RegisterCmdTable[i].CmdCbFunc = in_cmd->CmdCbFunc;
			DBG_Online_RegisterCmdTable[i].help = in_cmd->help;
			strncpy(DBG_Online_RegisterCmdTable[i].Name,in_cmd->Name,\
						sizeof(DBG_Online_RegisterCmdTable[i].Name)-1);
			DBG_Online_RegisterCmdTable[i].used = 1;
			DBG_Online_RegisterCmdTable[i].next = NULL;
			pthread_mutex_unlock(&DBG_Online_CmdTableLock);
			return 0;
		}
	}
	pthread_mutex_unlock(&DBG_Online_CmdTableLock);
		
	LOG_ERROR(" Cmd Table is FULL!!\n");
	return -1;
		
}

int Debug_Online_Server_DelCmd(const char *in_cmd_name)
{
	int i = 0;
	
	if((in_cmd_name == NULL) || !strlen(in_cmd_name))
	{
		LOG_ERROR(" Invalid Arg!!\n");
		return -1;
	}

	pthread_mutex_lock(&DBG_Online_CmdTableLock);

	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		if((DBG_Online_RegisterCmdTable[i].used == 1) 
			&& (strcasecmp(in_cmd_name, DBG_Online_RegisterCmdTable[i].Name) == 0))
		{
			LOG_ERROR(" Delete Cmd %s!!\n", in_cmd_name);
			memset(DBG_Online_RegisterCmdTable[i].Name, 0, DEBUG_ONLINE_CMD_NAME_LENGTH);
			DBG_Online_RegisterCmdTable[i].CmdCbFunc = NULL;
			DBG_Online_RegisterCmdTable[i].help = NULL;
			DBG_Online_RegisterCmdTable[i].used = 0;
			DBG_Online_RegisterCmdTable[i].next = NULL;
			pthread_mutex_unlock(&DBG_Online_CmdTableLock);
			return 0;
		}
	}
	pthread_mutex_unlock(&DBG_Online_CmdTableLock);
	LOG_ERROR(" Cmd %s not find to delete!!\n", in_cmd_name);

	return -1;	
}
	

static int Debug_Online_Server_ExecCmd(int argc, char *argv[])
{
	int i = 0;

	if(argv == NULL)
	{
		LOG_ERROR(" Invalid Arg!!\n");
		return -1;
	}


	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		if((DBG_Online_RegisterCmdTable[i].used == 1) && (strcasecmp(argv[0], DBG_Online_RegisterCmdTable[i].Name) == 0))
		{
			LOG_ERROR("\n Find The CMD Cmd Name:%s\n", DBG_Online_RegisterCmdTable[i].Name);
			if(DBG_Online_RegisterCmdTable[i].CmdCbFunc != NULL)
			{
				LOG_ERROR("\n");
				DBG_Online_RegisterCmdTable[i].CmdCbFunc(argc, argv);
				return 0;
			}
			
		}
	}
	
	return -1;
}

static void Debug_Online_Server_CmdHelp(int argc, char *argv[])
{
	int i = 0;
	int j=1;
	
	
	// arg1=help, arg2 = component name
	if(argc == 2  )
	{
		for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
		{
			if( DBG_Online_RegisterCmdTable[i].used  && \
				!strcasecmp(DBG_Online_RegisterCmdTable[i].Name,argv[1]) )
			{
				LOG_ERROR("\n");
				DBG_Online_RegisterCmdTable[i].help();
				return;
			}
		}	
	}

	LOG_ERROR("\nUsage: \n");
	LOG_ERROR("(1)\"help COMPONENT\" to get specific component help info!!\n");
	LOG_ERROR("   COMPONENT available:\n");
	j=1;
	for(i=0; i<DEBUG_ONLINE_CMD_COUNT; i++)
	{
		if( DBG_Online_RegisterCmdTable[i].used)
			{
				LOG_ERROR("   [%d]%s\n",j++,DBG_Online_RegisterCmdTable[i].Name);
			}
	}
	LOG_ERROR("\n");
	LOG_ERROR("   For example: \"help trace\" will get COMPONENT \"trace\" help info\n");
	LOG_ERROR("(2)\"COMPONENT arg1 arg2 ....argN\" to execute specific COMPONENT function\n");
	LOG_ERROR("   For example: \"trace ttk 5\" to exec COMPONENT \"trace\" with arg1=ttk, arg2=5\n");
	LOG_ERROR("(3)\"quit\" to exit!!\n");

	return;

}

static int Debug_Server_TransInputToCmd(char *in_buff, int *argc, char *argv[])
{
	int i = 0;
	char *p1, *p2;
	char tmp_buf[256];
	int space_num = 0;
	int len = 0;
	
	//LOG_ERROR("Debug_Client_TransInputToCmd In!!\n");
	if((in_buff == NULL) || (argc == NULL) || (argv == NULL))
	{
		LOG_ERROR(" Invalid arg pointer!!!\n");
		return -1;
	}

	strcpy(tmp_buf, in_buff);
	len = strlen(in_buff) + 1;

	//LOG_ERROR("len = %d, argc = %d\n", len, *argc);
	if(tmp_buf[0] == ' ')
	{
		LOG_ERROR(" CMD should not begin with space!!\n");
		return -1;
	}
	
	p1 = p2 = tmp_buf;
	
	for(i=1; i<len; i++)
	{
		if(tmp_buf[i] == ' ')
		{
			tmp_buf[i] = 0;

			p1 = tmp_buf + i + 1;
			if((p1 - p2)<2)
			{
				p2 = p1;
				continue;
			}

			space_num++;
		
			//LOG_ERROR("%s ", p2);
			if((strlen(p2) > 31) || (space_num > 7))
			{
				LOG_ERROR(" Invalid arg, too long\n");
				return -1;
			}
			strcpy(argv[space_num - 1], p2); 
			
			p2 = p1;
 
		}
	}

	if((strlen(p1) > 31) || (space_num > 7))
	{
		LOG_ERROR(" Invalid arg, too long\n");
		return -1;
	}

	strcpy(argv[space_num], p1);
	
	*argc = space_num + 1;

	//for(i=0; i< (*argc); i++)
		//LOG_ERROR("%s ", argv[i]);

	return 0;
}

//becareful: this function is unreenterable!!!
void Print2Client(const char* str, ...)
{
	static char temp[256];
	va_list va;
	va_start(va, str);
	vsnprintf(temp, sizeof(temp), str, va);
	va_end(va);
	if(cur_sock>0)
	{
		if(send(cur_sock,temp,strlen(temp),0) == -1)
			puts(temp);
	}
	else
		puts(temp);
}

static void* Debug_Online_Server(void *arg)
{
	int sockfd = 0;
	struct sockaddr_in s_addr;
	int len = 0;
	unsigned char buff[256];
	Debug_Online_Cmd_t *recv_cmd;

	int argc;
	char argv[8][32];
	char *pargv[8];
	int i = 0;
	int length;

	for(i=0; i<8; i++)
	{
		pargv[i] = &(argv[i][0]);
	}
	
	if( (sockfd = socket(AF_INET,SOCK_STREAM,0))  == -1 )
	{
		LOG_ERROR(" error!Create socket %s\n",strerror(errno));
		return NULL;
	}

	/*To avoid the error "Address already in use"*/
	int on=1;
	if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
	{
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	bzero(&s_addr,sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(DEBUG_ONLINE_PORT);

	if( bind(sockfd, (struct sockaddr *)(&s_addr),sizeof(struct sockaddr)) == -1 )
	{
		LOG_ERROR(" error!bind:%s\n",strerror(errno));
		close(sockfd);
		return NULL;
	}

	length = sizeof(s_addr); 
	if ( getsockname(sockfd, (struct sockaddr *)&s_addr, (socklen_t *)&length) < 0 )
	{
		LOG_ERROR(" error!getting socket name\n");	
		return NULL;
	}

	prctl(PR_SET_NAME, "DvrDebugOnline", 0, 0, 0);

	LOG_ERROR("DebugOnline socket port #%d, IP:%s\n", ntohs(s_addr.sin_port), inet_ntoa(s_addr.sin_addr)); 
	
	if( listen(sockfd, 5) == -1 )
	{
		LOG_ERROR(" error!listen:%s\n",strerror(errno));
		close(sockfd);
		return NULL;
	}

	LOG_ERROR("DebugOnline server start!!(pid: %d)\n", getpid());
	
	while(1)
	{
		int r_fd = 0;
		unsigned int c_addr_len = 0;
		struct sockaddr_in c_addr;
		bzero(&c_addr,sizeof(c_addr));
		
		if( (r_fd = accept(sockfd,(struct sockaddr *)&c_addr,&c_addr_len)) == -1 )
		{
			LOG_ERROR(" error!accept:%s\n",strerror(errno));
			close(sockfd);
			return NULL;
		}
		LOG_ERROR("DebugOnline client port #%d, IP:%s\n", ntohs(c_addr.sin_port), inet_ntoa(c_addr.sin_addr));
		cur_sock = r_fd;
		while(1)
		{
			if( (len= recv(r_fd, buff, 256, 0))<0)
			{
				LOG_ERROR(" error!read:%s\n",strerror(errno));
				continue;
			}

			if(len == 0)
			{
				cur_sock = -1;
				LOG_ERROR("\n Close connect!!\n");
				break;
			}
			//LOG_ERROR(" read len: %d\n",  len);

			recv_cmd = (Debug_Online_Cmd_t *)buff;
			//LOG_ERROR("\n Receive cmd:%s\n", recv_cmd->cmdbuf);
			//LOG_ERROR(" Receive cmd length:%x\n\n", recv_cmd->cmdlen);
			
			if( ((strlen(recv_cmd->cmdbuf) + 1)!= recv_cmd->cmdlen) &&
				((strlen(recv_cmd->cmdbuf) + 1)!= htonl(recv_cmd->cmdlen)) )
			{
				LOG_ERROR(" Server receive data from client error!! try again!\n");
				continue;
			}
			
			argc = 0;
			memset(argv, 0, sizeof(argv));

			if(Debug_Server_TransInputToCmd(recv_cmd->cmdbuf, &argc, pargv) == -1)
			{
				LOG_ERROR(" Invalid Cmd string!! Input again!!\n");
				continue;
			}
		
			if( strcasecmp(argv[0], DEBUG_ONLINE_STRING_QUIT) == 0)
			{
				break;
			}
			else if(strcasecmp(argv[0], DEBUG_ONLINE_STRING_HELP) == 0)
			{
				Debug_Online_Server_CmdHelp(argc,pargv);
			}
			else 
			{
				if(Debug_Online_Server_ExecCmd(argc, pargv) == -1)
				{
					//LOG_ERROR("Cmd not find!!\n");
					
					Debug_Online_Server_CmdHelp(1,pargv);
				}
			}
			
			memset(buff, 0, 256);
		}
		close(r_fd);
		LOG_ERROR("\n Wait next connecton!!\n");
		
	}

	LOG_ERROR("\n/*******************************/\n");
	LOG_ERROR("Debug Online SERVER quit!!!!!!!!!!\n");
	LOG_ERROR("\n/*******************************/\n");

	close(sockfd);
	
	return NULL;
}


//this function have reenterable issue!!
void Debug_Online_Server_Start(void)
{
	int res=0;
	pthread_t debug_online_thread;

	static int bFirst =1;
	if(bFirst == 1)
	{
		bFirst = 0;
		Debug_Online_Server_InitCmdList();

		/**
		  * this is a special component that register by DEBUGONLINE itself, other component can regiter 
		  * command in their own module
		  **/
		
		LOG_INFO("Create debug online server pthread here!!!!\n");
		res = pthread_create(&debug_online_thread,NULL,Debug_Online_Server,NULL);
		if(res!=0)
		{
			LOG_ERROR("ERROR: Can't start debug online server\n");
		}
		LOG_INFO("Create debug online server pthread successfullly!!!!\n");
	}
}

#endif //#ifdef __ONLINE_DEBUG_ENABLE__

