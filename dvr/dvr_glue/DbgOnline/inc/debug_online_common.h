#ifndef _DEBUG_ONLINE_H
#define _DEBUG_ONLINE_H

//***************************************************************
//
// Macro Define
//
//***************************************************************

#define DEBUG_ONLINE_ENV	"ENABLE_DEBUG_ONLINE"
#define DEBUG_ONLINE_PORT 	52113
#define DEBUG_ONLINE_LISTENADDR "127.0.0.1"

#define DEBUG_ONLINE_CONNECT_RETRY_TIMES 3
#define DEBUG_ONLINE_CMD_NAME_LENGTH 8

#define DEBUG_ONLINE_CMD_LENGTH 252

#define DEBUG_ONLINE_CMD_COUNT 32


#define DEBUG_ONLINE_PROMPT "dbg_online>>"
#define DEBUG_ONLINE_STRING_QUIT "quit"
#define DEBUG_ONLINE_STRING_HELP "help"

//***************************************************************
//
// Data Type Define
//
//***************************************************************

typedef struct dbg_online_cmd{
	unsigned int cmdlen;
	char cmdbuf[DEBUG_ONLINE_CMD_LENGTH];
}Debug_Online_Cmd_t; //data type to store the cmd send to server


typedef int (*Debug_Online_CbCmdFunc)(int argc, char *argv[]);
typedef void (*HelpFunc)(void);
typedef struct dbg_online_CbCmdParam{
	const char *Name;//[DEBUG_ONLINE_CMD_NAME_LENGTH];
	Debug_Online_CbCmdFunc CmdCbFunc;
	HelpFunc help;
}Debug_Online_CbCmdParam_t;


//*************************************************************
// Function
//*************************************************************

void Debug_Online_Server_InitCmdList(void);
int Debug_Online_Server_RegistCmd(const Debug_Online_CbCmdParam_t *in_cmd);
int Debug_Online_Server_DelCmd(const char *in_cmd_name);
void Debug_Online_Server_Start(void);
void Print2Client(const char* str, ...);

//*************************************************************
// Global Data
//*************************************************************

#endif

