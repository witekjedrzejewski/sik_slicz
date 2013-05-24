/* custom error codes */

#ifndef _SIK_ERROR_CODES_H_
#define	_SIK_ERROR_CODES_H_

#define OK 0
#define ERR_PORT_NONE 1
#define ERR_RECV_NONE 2
#define ERR_VLAN_NONE 3
#define ERR_ADDR_WRONG 4
#define ERR_VLAN_BOUNDS 5
#define ERR_VLAN_DUP 6
#define ERR_VLAN_MANY_U 7
#define ERR_SOCK 8
#define ERR_TOO_MANY 9

/* generates error message given code */
char* get_error_message(int error_code);

#endif

