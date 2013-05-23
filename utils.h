#ifndef _SIK_UTILS_H_
#define	_SIK_UTILS_H_

/* checks if text starts with prefix */
int starts_with(char* text, char* prefix);

/* fills sockaddr basing on host and port, returns err_code */
int sockaddr_from_host_port(char* host, char* port, 
				struct sockaddr_in* result);

#endif

