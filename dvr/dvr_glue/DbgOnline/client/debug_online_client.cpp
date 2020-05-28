#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#if 1
typedef unsigned int dvr_bool;
#define dvr_false 0
#define dvr_true  1
#endif 

#include "../inc/debug_online_common.h"
#define ETU_CL_NUM_LINES 8       // Number of lines to keep
//#define CTRL(c) ((c)&0x1F)
#define CHAR_ESCAPE  0x1B
#define CHAR_BELL    0x07
#define cmd_history dvr_false
//static int sockfd = -1;

static unsigned char ServerAddr[64] = {'\0'};


static int Debug_Client_GetOneLine(char *buf, int buflen, int timeout)
{
	static char etu_cl_lines[16][256];
    char *ip = buf;   // Insertion point
    char *eol = buf;  // End of line    
    char c='\0';
    dvr_bool res = dvr_false;
    static char last_ch = '\0';
    int _timeout;
	//static char etu_cl_lines[ETU_CL_NUM_LINES][ETU_CMDLINE_LEN]; 
	
#if 1
//
// Command line history support
//    ^P - Select previous line from history
//    ^N - Select next line from history
//    ^A - Move insertion [cursor] to start of line
//    ^E - Move cursor to end of line
//    ^B - Move cursor back [previous character]
//    ^F - Move cursor forward [next character]
//
    
	//static char _cl_lines[ETU_CL_NUM_LINES][ETU_CMDLINE_LEN];
    static int  etu_cl_index = -1;      // Last known command line
    static int  etu_cl_max_index = -1;  // Last command in buffers
    int index = etu_cl_index;  // Last saved line
    char *xp;
#endif

    // Display current buffer data
    /*while (*eol){
		putchar(*eol++);
    }*/
    
    ip = eol;   
    
    while (dvr_true) {

        if ((timeout > 0) && (eol == buf)) {
#define MIN_TIMEOUT 50
            _timeout = timeout > MIN_TIMEOUT ? MIN_TIMEOUT : timeout;
            //mon_set_read_char_timeout(_timeout);
            while (timeout > 0) {
                //res = mon_read_char_with_timeout(&c);
				res = dvr_true;
				c = getchar();
                if (res) {
                    // Got a character
                    //do_idle(dvr_false);
                    break;
                }
                timeout -= _timeout;
            }
            if (res == dvr_false) {
                //do_idle(dvr_true);
                return dvr_false;  // Input timed out
            }
        } else {
            //mon_read_char(&c);
			c = getchar();
        }
        *eol = '\0';       
          

        
#if 1
          if (c == CHAR_ESCAPE)   // character = escape
          {
              //sleep(1);
              //mon_read_char(&c);
			  c = getchar();
              if ( c == 0x5B ) // character = '['
              {
                  //sleep(1);
                  //mon_read_char(&c);
				  c=getchar();
                  switch (c)
                  {
                  case 'A':                 
                      c = CTRL('P');    // Up Arrow = '^P' = 'ESC'  '['  'A'
                      break;
                      
                  case 'B':                  
                      c = CTRL('N');    // Down Arrow = '^N' = 'ESC'  '['  'B'
                      break;

                  case 'C':
                      c = CTRL('F');    // Right Arrow = '^F' = 'ESC'  '['  'C'
                      break;
                      
                  case 'D':
                      c = CTRL('B');    // Left Arrow = '^B' = 'ESC'  '['  'D'
                      break;
                      
                  default:
                      c = CHAR_BELL;
                      break;
                      
                  }
              }
              else
              {
                  c = CHAR_BELL;
              }
          }          
#endif
        switch (c) {
#if 1       
        case CTRL('P'):           
            // Fetch the previous line into the buffer            
            if (index >= 0) {
                // Erase the previous line [crude]               
                while (ip < eol)
                {
                    //mon_write_char(' ');
					putchar(' ');
                    ip++;  // move to  the end of string                    
                }
                
                while (ip != buf) {
                    //mon_write_char('\b');
					putchar('\b');
                    //mon_write_char(' ');
					putchar(' ');
                    //mon_write_char('\b');
					putchar('\b');
                    ip--;
                }
                strcpy(buf, etu_cl_lines[index]);
                while (*ip) {
                    //mon_write_char(*ip++);
					putchar(*ip++);
                }
                eol = ip;                
                // Move to previous line
                index--;
                if (index < 0) {
                    index = etu_cl_max_index;
                }
            } else {
                //mon_write_char(0x07);  // Audible bell on most devices
				putchar(0x07);
            }
            break;
        case CTRL('N'):
            // Fetch the next line into the buffer
            if (index >= 0) {
                if (++index > etu_cl_max_index) index = 0;
                // Erase the previous line [crude]                
                while (ip < eol)
                {
                    //mon_write_char(' ');
					putchar(' ');
                    ip++;  // move to  the end of string
                }
                
                while (ip != buf) {
                    //mon_write_char('\b');
					putchar('\b');
                    //mon_write_char(' ');
					putchar(' ');
                    //mon_write_char('\b');
					putchar('\b');
                    ip--;
                }
                strcpy(buf, etu_cl_lines[index]);
                while (*ip) {
                    //mon_write_char(*ip++);
					putchar(*ip++);
                }
                eol = ip;                
            } else {
                //mon_write_char(0x07);  // Audible bell on most devices
				putchar(0x07);
            }
            break;
            
        case CTRL('B'): 
            // Move insertion point backwards            
            if (ip != buf) {
                //mon_write_char('\b');
				putchar('\b');
                ip--;
            }
            break;
        case CTRL('F'):
            // Move insertion point forwards
            if (ip != eol) {
                //mon_write_char(*ip++);
				putchar(*ip++);
            }
            break;
            
        case CTRL('E'):
            // Move insertion point to end of line
            while (ip != eol) {
                //mon_write_char(*ip++);
				putchar(*ip++);
            }
            break;
        case CTRL('A'):
            // Move insertion point to beginning of line           
            if (ip != buf) {
                xp = ip;
                while (xp-- != buf) {                 
                        //mon_write_char('\b');
						putchar('\b');
                }
            }
            ip = buf;
            break;
        case CTRL('K'):
            // Kill to the end of line
            if (ip != eol) {
                xp = ip;
                while (xp++ != eol) {
                    //mon_write_char(' ');
					putchar(' ');
                }
                while (--xp != ip) {
                    //mon_write_char('\b');
					putchar('\b');
                }
                eol = ip;               
            }
            break;
                    
        case CTRL('D'):
            // Erase the character under the cursor
            if (ip != eol) {
                xp = ip;
                eol--;
                while (xp != eol) {
                    *xp = *(xp+1);
                    //mon_write_char(*xp++);
					putchar(*xp++);
                }
                //mon_write_char(' ');  // Erases last character
				putchar(' ');
                //mon_write_char('\b');
				putchar('\b');
                while (xp-- != ip) {
                    //mon_write_char('\b');
					putchar('\b');
                }
            }
            break;
            // nop service for not supported CTRL sequence        
        case CTRL('I'):       
        case CTRL('L'):       
        case CTRL('O'):
        case CTRL('R'):
        case CTRL('S'):
        case CTRL('T'):
        case CTRL('W'):
        case CTRL('Q'):
        case CTRL('U'):
        case CTRL('X'):
        case CTRL('Y'):            
        case CTRL('Z'):    
            break;
            
#endif // CYGNUM_REDBOOT_CMD_LINE_EDITING
        case CTRL('C'): // ^C
            // Abort current input
            printf("^C\n");
            *buf = '\0';  // Nothing useful in buffer
            return dvr_false;
        case '\n':
        case '\r':
            // If previous character was the "other" end-of-line, ignore this one
            if (((c == '\n') && (last_ch == '\r')) ||
                ((c == '\r') && (last_ch == '\n'))) {
                c = '\0';
                break;
            }
            // End of line
	    //if (console_echo) {
		if (0) {
                //mon_write_char('\r');
				putchar('\r');
			
                //mon_write_char('\n');
				putchar('\n');
	    }
            last_ch = c;
#if 0
            if ((cmd_history == dvr_true) && (buf != eol)) {
			
                // Save current line - only when enabled
                if (++etu_cl_index == ETU_CL_NUM_LINES) etu_cl_index = 0;
                if (etu_cl_index > etu_cl_max_index) etu_cl_max_index = etu_cl_index;
                strcpy(etu_cl_lines[etu_cl_index], buf);
            }
#endif
            return dvr_true;                        

        case 0x00:
            break;

        case CHAR_BELL:
            //mon_write_char(CHAR_BELL);
			putchar(CHAR_BELL);
            break;
            
        case '\b':        
            if (ip != buf) {
#if 1
                if (ip != eol) {
                    ip--;
                    //mon_write_char('\b');
					putchar('\b');
                    xp = ip;
                    while (xp != (eol-1)) {
                        *xp = *(xp+1);
                        //mon_write_char(*xp++);
						putchar(*xp++);
                    }
                    //mon_write_char(' ');  // Erases last character
					putchar(' ');
                    //mon_write_char('\b');
					putchar('\b');
                    while (xp-- != ip) {
                        //mon_write_char('\b');
						putchar('\b');
                    }
                } else {
                    //if (console_echo) {
					if (1) {
                        //mon_write_char('\b');
						putchar('\b');
                        //mon_write_char(' ');
						putchar(' ');
                        //mon_write_char('\b');
						putchar('\b');
                    }
                    ip--;
                }
                eol--;
#else
                if (console_echo) {
                    //mon_write_char('\b');
					putchar('\b');
                    //mon_write_char(' ');
					putchar(' ');
                    //mon_write_char('\b');
					putchar('\b');
                }
                ip--;
                eol--;
#endif
            }
            break;
#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
        case '+': // fall through
        case '$':
            if (ip == buf || last_ch != '\\')
            {
                // Give up and try GDB protocol
                ungetDebugChar(c);  // Push back character so stubs will see it
                return _GETS_GDB;
            }
            if (last_ch == '\\') {
#if 1
                if (ip == eol) {
                    // Just save \$ as $
                    eol = --ip;
                } else {
                    //mon_write_char('\b');
					putchar('\b');
                    *--ip = c;
                    //mon_write_char(c);
					putchar(c);
                    break;
                }
#else
                ip--;  // Save \$ as $
#endif
            }
            // else fall through
#endif
        default:
#if 1
            // If the insertion point is not at the end of line, make space for it
            if (ip != eol) {
                xp = eol;
                *++eol = '\0';
                while (xp != ip) {
                    *xp = *(xp-1);
                    xp--;
                }
            }
#endif
            //if (console_echo) {
			/*if (1) {
                //mon_write_char(c);
				putchar(c);
            }*/
            if (ip == eol) {
                // Advance both pointers
                *ip++ = c;
                eol = ip;
#if 1
            } else {
                // Just insert the character
                *ip++ = c;
                xp = ip;
                while (xp != eol) {
                    //mon_write_char(*xp++);
					putchar(*xp++);
                }
                while (xp-- != ip) {
                    //mon_write_char('\b');
					putchar('\b');
                }
#endif
            }
        }
        last_ch = c;
        if (ip == buf + buflen - 1) { // Buffer full
            *ip = '\0';
            return buflen;
        }
    }
}


static int Debug_Client_Init(struct sockaddr_in *out_s_addr, char *server_ip, short port_num, int *out_sockfd)
{
	int sockfd = 0;

	if((out_s_addr == NULL) || (out_sockfd == NULL))
	{
		printf("Invalid arg pointer!!!\n");
		return -1;
	}
	
	bzero(out_s_addr,sizeof(*out_s_addr));
	out_s_addr->sin_family = AF_INET;
	out_s_addr->sin_addr.s_addr = inet_addr(server_ip);//inet_addr(DEBUG_ONLINE_LISTENADDR);
	out_s_addr->sin_port = htons(port_num);//htons(DEBUG_ONLINE_PORT);
	
	
	if( (sockfd = socket(AF_INET,SOCK_STREAM,0))  == -1 )
	{
		printf("error!Create socket %s\n",strerror(errno));
		return -1;
	}

	*out_sockfd = sockfd;

	return 0;
}

static int Debug_Client_Connect(struct sockaddr_in *in_s_addr, int in_sockfd)
{
	int i;

	if(in_s_addr == NULL)
	{
		printf("Invalid arg pointer!!!\n");
		return -1;
	}

	for(i=0; i<DEBUG_ONLINE_CONNECT_RETRY_TIMES; i++)
	{
		if( (connect(in_sockfd,(struct sockaddr *)in_s_addr,sizeof(*in_s_addr))) == -1 )
		{
			printf("error!connect:%s!! sleep 1s and retry!!\n",strerror(errno));
			usleep(1000000);
			continue;
		}
		
		return 0;
	}

	return -1;
}

static int Debug_Client_SendCmd(int in_sockfd, unsigned char *in_buff, unsigned int in_len)
{
	int len = 0;

	if(in_buff == NULL)
	{
		printf("Invalid arg pointer!!!\n");
		return -1;
	}

	if( (send(in_sockfd, in_buff,in_len, 0))<0)
	{
		printf("error!write:%s\n",strerror(errno));
		return -1;
	}

	return 0;
}


//***************************************************
// signal process
//***************************************************
void CatchSignal(int sig)
{
	switch(sig)
	{
		case SIGTSTP:
			printf(" SIGTSTP\n");
			break;
		default:
			break;
	}
	//close(sockfd);
	kill(getpid(), SIGINT);
}

void SetSignalProcess(void)
{
	struct sigaction act;
	sigset_t mask_set;
	sigset_t old_set;
	
	//act.sa_handler = CatchSignal;
	//sigemptyset(&act.sa_mask);
	//act.sa_flags = 0;

	sigemptyset(&mask_set);

	if(sigaddset(&mask_set, SIGSTOP) == -1)
	{
		printf(" Add signal<1> error!!\n");
		return;
	}

	if(sigaddset(&mask_set, SIGTSTP) == -1)
	{
		printf(" Add signal<2> error!!\n");
		return;
	}

	if(sigprocmask(SIG_BLOCK, &mask_set, &old_set) == -1)
	{
		printf(" Set Block signal set error!!\n");
		return;
	}
	
	//sigaction(SIGTSTP, &act, 0);
	
	return;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in s_addr;
	int sockfd = -1;
	static int finish_init = 0;
	int i = 0, j = 0;
	dvr_bool prompt = dvr_true;
	unsigned char ch;
	char *server_ipaddr;
	short server_port;
	pid_t clpd=-1;

	char input_buff[256];
	int len = 0;

	Debug_Online_Cmd_t cmd;

	SetSignalProcess();

	if(argc < 2)
	{
		printf("USAGE: ./client server_ipaddr [server_port]\n");
		return 0;
	}

	server_ipaddr = argv[1];
	server_port = (argc==2)?DEBUG_ONLINE_PORT:atoi(argv[2]);
	if(Debug_Client_Init(&s_addr, server_ipaddr, server_port, &sockfd) == -1)
	{
		printf("Init Client socket addr and sockfd fail!!\n");
		return -1;
	}


	if(Debug_Client_Connect(&s_addr, sockfd) == -1)
	{
		printf("Connect to Server fail!!!\n");
		return -1;
	}

	printf("/*********************************************/\n");
	printf("/*        Debug Online                       */\n");
	printf("/*   Input \"help\" to get cmd info!!          */\n");
	printf("/*********************************************/\n");

	if((clpd = fork()) == -1)
	{
		printf("fork error\n");
		return -1;
	}
	else if(clpd ==0)
	{
		char buf[256];
		close(0);
		dup2(sockfd,0);
		while(1) puts(fgets(buf,256,stdin));
	}
	while(1)
	{
		len = 0;
		memset(input_buff, 0, sizeof(input_buff));	

		if (prompt)
		{
			printf("dbg_online>>");
			prompt = dvr_false;
		}

		Debug_Client_GetOneLine(input_buff,256,0);
		
		if(!strlen(input_buff)){
			prompt = dvr_true;
			continue;	
		}
		
		input_buff[strlen(input_buff)+1] = 0;
		len = strlen(input_buff);

		memset(&cmd, 0, sizeof(cmd));
		cmd.cmdlen = htonl(len + 1);
		strcpy(cmd.cmdbuf, input_buff);
		if(Debug_Client_SendCmd(sockfd, (unsigned char *)&cmd, sizeof(cmd)) == -1)
		{
			printf(" Send cmd to server error!!\n");
		}	

		if( strcmp(input_buff, "quit") == 0)
		{
			if(kill(clpd,SIGTERM)==0);
			if((waitpid(clpd,NULL,WNOHANG))==0);
			break;
		}		
		
		usleep(200000);
		prompt = dvr_true;
		
	}
	close(sockfd);
	
}

