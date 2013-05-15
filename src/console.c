/**
 * @file
 * @brief	console simulate
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>
#include <serial.h>

/*
 *  ANSI
 * 
 * \033[A			cursor up
 * \033[B			cursor down
 * \033[C			cursor right
 * \033[D			cursor left
 * 
 */
#define putchar			serial_putc
#define raw_getchar		serial_getc


///* test if ctrl-c was pressed */
int ctrlc (void)
{           
        if (serial_tstc ()) {
            switch (serial_getc()) {
            case 0x03:      /* ^C - Control C */
                return 1;
            default: 
                break;
            }
        }   
   
    return 0;
}

/*-------------------------*/
//static int cmd_echo(int argc, char** argv)
//{
//	int i;
//	
//	for (i=0; i<argc; i++) {
//		PRINT("%s ", argv[i]);
//	}
//	PRINT("\n");
//	return 0;
//}
//
//INSTALL_CMD(echo, cmd_echo, "echo");

/*-------------------------*/

#define CMD_HST_DEP			16 // should be 2^n aligned,  history depth is 16
#define MAX_CMD_LEN  		256
#define MAX_CMD_ARG   		8
#define MAX_CMD_ARG_LEN		32

#define CMD_ADJUST_INDEX(index)	((index) & (CMD_HST_DEP - 1))

static char cmdArgs[MAX_CMD_ARG][MAX_CMD_ARG_LEN];

struct CmdStack
{
	char *vpchCmdStack[CMD_HST_DEP];
	int	  iCmdHstSP;		//history position
};

static char gCmdBuffer[CMD_HST_DEP][MAX_CMD_LEN];

static struct CmdStack gCmdStack;
static struct CmdStack *g_pCmdStack = NULL;


static void CmdBackspace(void)	//???
{
	PRINT("\033[D\033[1P");
}

static void ShowPrompt(void)
{	
	PRINT("Innofidei: # ");
}

/*
 * @param input_c	the character which to be insert to buffer
 * @param cur_pos	the buffer position which the character to be inserted
 * @param cur_max	the last character position in buffer
 */
static void InsertOneKey(char input_c, char *buf, int *cur_pos, int *cur_max)
{
	int i;

	for (i = *cur_max + 1; i > *cur_pos; --i)
		buf[i] = buf[i - 1];
	buf[i] = input_c;
	++(*cur_pos);
	++(*cur_max);

	PRINT("\033[4h");		//set insert mode
	putchar(input_c);
	PRINT("\033[4l");		//???
}

static void BackSpaceOneKey(char *buf, int *cur_pos, int *cur_max)
{
	int i;

	for (i = *cur_pos; i < *cur_max; ++i)
		buf[i - 1] = buf[i];
	buf[*cur_max] = '\0';
	--(*cur_max);
	--(*cur_pos);

	CmdBackspace();
}

static void DeleteOneKey(char *buf, int *cur_pos, int *cur_max)
{
	int i;

	if (*cur_pos == *cur_max)
		return;

	for (i = *cur_pos; i < *cur_max - 1; ++i)
		buf[i] = buf[i + 1];
	buf[*cur_max] = '\0';

	--(*cur_max);

	PRINT("\033[1P");
}

static void PrintInput(char input_c, char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos < MAX_CMD_LEN - 1) {
		InsertOneKey(input_c, buf, cur_pos, cur_max);
	} else {
		putchar('\n');
		PRINT("error: command too long\nthe command must be less than %d letter", MAX_CMD_LEN);
		putchar('\n');
		ShowPrompt();
		PRINT(buf);
	}
}

static int CmdUpKey(char *buf, int *cur_pos, int *pindex, int *cur_max)
{
	int i;
	int index = *pindex;
	int lpos = *cur_pos;

	//save current buffer, but history index does not increase
	if (index == g_pCmdStack->iCmdHstSP) {
		buf[lpos] = '\0';

//		if (NULL == g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP]) {
//			if (NULL == (g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP] = (char *)malloc(MAX_CMD_LEN))) {
//				PRINT("ERROR: fail to malloc!\n");
//				return -1;
//			}
//		}
		strcpy(g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP], buf);
	}

	index = CMD_ADJUST_INDEX(index - 1);

	if (index != g_pCmdStack->iCmdHstSP && NULL != g_pCmdStack->vpchCmdStack[index]) {
		// erase the command on screen
		for (i = 0; i < lpos; i++) {
			buf[i] = '\0'; //erase input buffer
			CmdBackspace();  
		}

		// show history command
		PRINT("%s", g_pCmdStack->vpchCmdStack[index]);

		//update buffer & index
		strcpy(buf, g_pCmdStack->vpchCmdStack[index]);
		*pindex = index;
		*cur_pos = strlen(g_pCmdStack->vpchCmdStack[index]);
		*cur_max = *cur_pos;

		PRINT("\033[0K");	//erase from the active  position to the end of the line.

		return 0;
	}

	return -1;
}

static int CmdDownKey(char *buf, int *cur_pos, int *pindex, int *cur_max)
{
	int i;
	int index = *pindex;
	int lpos = *cur_pos;

	if (index == g_pCmdStack->iCmdHstSP)
		return 0;

	index = CMD_ADJUST_INDEX(index + 1);

	// erase the command on screen
	for (i = 0; i < lpos; i++) {
		buf[i] = '\0'; //erase input buffer
		CmdBackspace();  
	}

	// show history command
	PRINT("%s", g_pCmdStack->vpchCmdStack[index]);

	//update buffer & index
	strcpy(buf, g_pCmdStack->vpchCmdStack[index]);
	*pindex = index;
	*cur_pos = strlen(g_pCmdStack->vpchCmdStack[index]);
	*cur_max = *cur_pos;

	PRINT("\033[0K");

	return 0;
}

static int CmdRightKey(char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos < *cur_max) {
		++(*cur_pos);
		PRINT("\033[C");
	}

	return 0;
}

static int CmdLeftKey(char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos > 0) {
		--(*cur_pos);
		PRINT("\033[D");
	}

	return 0;
}

static int CmdUpdateHistory(const char *buf)
{
//	if (NULL == g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP]) {
//		if (NULL == (g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP] = (char *)malloc(MAX_CMD_LEN))) {
//			PRINT("ERROR: fail to malloc!\n");
//			return -1;
//		}
//	}

	strcpy(g_pCmdStack->vpchCmdStack[g_pCmdStack->iCmdHstSP], buf);

	g_pCmdStack->iCmdHstSP = CMD_ADJUST_INDEX(++g_pCmdStack->iCmdHstSP);

	return 0;
}

static int GetMessage(char *buf)
{
	char input_c;
	int cur_pos = 0;
	int cur_max = 0;
	int hst_pos = g_pCmdStack->iCmdHstSP;
	int esc_sequence = 0;
	int spec_key = 0;
	int ret = 0;
	
	memset(buf, '\0', MAX_CMD_LEN);

	while (0 == cur_max) {
		input_c = 0;

		ShowPrompt();
		while (input_c != '\r' && input_c != '\n') {
			input_c = raw_getchar();
			//fixme: support F1 F2 F3 ...
			if (input_c == '\033') {		//escape
				spec_key = 1;
				input_c = raw_getchar();
				if (input_c == '[') {
					esc_sequence = 1;
					input_c = raw_getchar();
				}
			} else {
				spec_key = 0;
				esc_sequence = 0;
			}

			switch (input_c) {
			case '\r':
			case '\n':
				putchar('\n');
				break;

			case 0x03: 		// ctrl + c
				cur_pos = 0;
				cur_max = 0;
				input_c = '\n';
				putchar(input_c);
				break;

			case 0x08:		//backspace
			case 0x7f:		//delete
				if (cur_pos > 0) {
					BackSpaceOneKey(buf, &cur_pos, &cur_max);
				}
				break;

			case 'A':
			case 'B':// no filter: escape +' [' + 'A'
				if (1 == esc_sequence) {
					if ('A' == input_c) 
						ret = CmdUpKey(buf, &cur_pos, &hst_pos, &cur_max);
					else
						ret = CmdDownKey(buf, &cur_pos, &hst_pos, &cur_max);
				} else { // not a up/down key, treat it as a normal input
					PrintInput(input_c, buf, &cur_pos, &cur_max);
				}

				break;

			case 'C':
			case 'D':
				if (1 == esc_sequence) {
					if ('C' == input_c)
						ret = CmdRightKey(buf, &cur_pos, &cur_max);
					else
						ret = CmdLeftKey(buf, &cur_pos, &cur_max);
					break;
				} else { // not a left/right key, treat it as a normal input
					PrintInput(input_c, buf, &cur_pos, &cur_max);
				}

				break;

			case 'O':
				if (1 == spec_key) {
					input_c = raw_getchar();

					if ('H' == input_c) {
						if (cur_pos != 0)
							PRINT("\033[%dD", cur_pos);
						cur_pos = 0;				
					}
					if ('F' == input_c) {
						if (cur_pos != cur_max)
							PRINT("\033[%dC", cur_max - cur_pos);
						cur_pos = cur_max;
					}
				} else { // not a Home/End key, treat it as a normal input
					PrintInput(input_c, buf, &cur_pos, &cur_max);
				}

				break;

			case '3':
				if (1 == spec_key) {
					input_c = raw_getchar();

					if ('~' == input_c) {
						DeleteOneKey(buf, &cur_pos, &cur_max);
					}
				} else {
					PrintInput(input_c, buf, &cur_pos, &cur_max);
				}

				break;

			default:
				if (input_c >= 0x20 && input_c <= 0x7f) //filter the character
					PrintInput(input_c, buf, &cur_pos, &cur_max);

				break;
			}
		}
	}

	buf[cur_max] = '\0';

	CmdUpdateHistory(buf);

	return 1;
}

static int TranslateMessage(char *buf, char *argv[])
{
	int len, num = 0;
	char tmp_buf[MAX_CMD_LEN];

	while (*buf && num < MAX_CMD_ARG) {
		while (*buf && ' ' == *buf) //remove the space character
			buf++;

		if (!*buf)
			break;

		len = 0;
		while (*buf && *buf != ' ') {
			*(tmp_buf + len) = *buf++;
			len++;
		}

		tmp_buf[len] = '\0';
		
		if (len + 1 > MAX_CMD_ARG_LEN) {
			PRINT("\nERROR: argument is too long! "
					"should not exceed %d bytes\n", MAX_CMD_ARG_LEN - 1);
			return -1;
		}
		argv[num] = &(cmdArgs[num][0]);

		strcpy(argv[num], tmp_buf);
		argv[num][len] = '\0';

		num++;
	}

	return num;
}

static int DispatchMessage(int argc, char *argv[])
{
	cmd_tbl_t *cmdtp;
		
	cmdtp = find_cmd(argv[0]);
	if (cmdtp != NULL){
		cmdtp->cmd(argc, argv);
	}else{
		PRINT(" Command %s - Invalid Command!\n Please use \"help\" to get command list.\n", argv[0]);
	}

	putchar('\n');
	
	return 0;
}

static int CreateCmdStack(struct CmdStack **pCmdStack)
{
	int i;
	
	memset(&gCmdStack, 0, sizeof(gCmdStack));
	*pCmdStack = (struct CmdStack *)&gCmdStack;

	memset(gCmdBuffer, 0, sizeof(gCmdBuffer));
	for (i=0; i<CMD_HST_DEP; i++ ) {
		(*pCmdStack)->vpchCmdStack[i] = &gCmdBuffer[i][0];
	}
	return 0;
}

void start_console(void)
{
	char szMsg[MAX_CMD_LEN];
	int ret;
	
	ret = CreateCmdStack(&g_pCmdStack);
	if (ret)
		return;
	
	while (GetMessage(szMsg)) {
		int   argc;
		char *argv[MAX_CMD_ARG];

		argc = TranslateMessage(szMsg, argv);
		if (argc < 0)
			continue;

		DispatchMessage(argc, argv);
	}

}
