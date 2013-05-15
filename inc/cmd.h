#ifndef COMMANDS_H_
#define COMMANDS_H_

struct cmd_tbl_s {
	char*	name;	/**< command name */
	int		(*cmd)(int argc, char** argv);		/**< command implemention function */
	char*	help;	/**< help message */
};

typedef struct cmd_tbl_s	cmd_tbl_t;

cmd_tbl_t* find_cmd (const char* cmd);


#define INSTALL_CMD(name, cmd, help)	\
cmd_tbl_t __cmd_##name __attribute__((used, section(".cmdcall.entry"))) = {	\
		#name,	\
		cmd,	\
		help,	\
}

#endif /*COMMANDS_H_*/

