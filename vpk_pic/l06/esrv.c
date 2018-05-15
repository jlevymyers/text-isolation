/*
 * Echo Protocol (TCP-based Echo Service)
 *  - RFC 862: https://tools.ietf.org/html/rfc862
 *
 * Vasileios P. Kemerlis <vpk@cs.brown.edu>
 *  - CSCI 1650: Software Security and Exploitation
 *  - https://cs.brown.edu/courses/csci1650/
 */

/*
 * Bugs:
 *  - Stack-based buffer overflow in 'cli_hndl()'
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define	BACKLOG_DFL	1	/* default backlog value		*/
#define ECHO_PORT_DFL	7777	/* default Echo Protocol port (TCP)	*/
#define	BUF_SZ		512	/* buffer size				*/


/* cleanup routine */
static void
cleanup(int srv_fd)
{
	/* socket cleanup */
	if (srv_fd != -1)
		close(srv_fd);
}

/* client handler */
static void
cli_hndl(int cfd)
{
	/* message buffer			*/
	char	buf[BUF_SZ];

	/* length pointer			*/
	ssize_t	len;

	/* cleanup				*/
	/* bzero(buf, BUF_SZ);			*/

	/*
	 * main processing loop; messages
	 *
	 * BUG: 'BUF_SZ<<1' (should be 'BUF_SZ' or less)
	 */
	while ((len = read(cfd, buf, BUF_SZ<<1)) > 0) {
		/*
		 * don't bother checking
		 * the return value :)
		 */
		write(cfd, buf, len);
	}

	/* error reporting			*/
	if (len == -1)
		perror("[-] read(2) failed");

	/* done					*/
}

int
main(int argc, char **argv)
{
	/* socket descriptors; server, client	*/
	int sfd = -1, cfd = -1;

	/* IPv4 addresses; server, client	*/
	struct sockaddr_in
		sin = {
			.sin_family	= AF_INET,
			.sin_port	= htons(ECHO_PORT_DFL),
			.sin_addr	= { INADDR_ANY },
		},
		cin = {
			.sin_family	= 0,
			.sin_port	= 0,
			.sin_addr	= { .s_addr = 0 },
		};

	/* client address size; value-result	*/
	socklen_t clen	= sizeof(struct sockaddr_in);

	/* bool (enable)			*/
	int enable	= 0xe4ff;
	
	/* verbose				*/
	fprintf(stdout, "[+] Creating listening socket... ");

	/* get the listening socket		*/
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		/* failed			*/
		fprintf(stdout, "[FAILURE]\n"); fflush(stdout);	
		perror("[-] socket(2) failed");
		goto err;
	}

	/* enable address reuse			*/
	if (setsockopt(sfd,
			SOL_SOCKET,
			SO_REUSEADDR,
			&enable,
			sizeof(int)) == -1)
		perror("[*] setsockopt(2) failed");

	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	
	fprintf(stdout, "[+] Binding listening socket.... ");

	/* bind the listening socket		*/
	if (bind(sfd,
		(const struct sockaddr *)&sin,
		sizeof(struct sockaddr_in)) == -1) {
		/* failed 			*/
		fprintf(stdout, "[FAILURE]\n"); fflush(stdout);	
		perror("[-] bind(2) failed");
		goto err;
	}	

	/* verbose 				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	
	fprintf(stdout, "[+] Listening at %s:%hu... ",
			inet_ntoa(sin.sin_addr),
			ntohs(sin.sin_port));

	/* mark the socket as passive		*/
	if (listen(sfd, BACKLOG_DFL) == -1) {
		/* failed			*/
		fprintf(stdout, "[FAILURE]\n"); fflush(stdout);	
		perror("[-] listen(2) failed");
		goto err;
	}
	
	/* verbose				*/
	fprintf(stdout, "[SUCCESS]\n"); fflush(stdout);	

	/* main processing loop; connections	*/
	while ((cfd = accept(sfd, (struct sockaddr *)&cin, &clen)) != -1) {
		/* verbose			*/
		fprintf(stdout, "[*] New connection from %s:%hu\n",
				inet_ntoa(cin.sin_addr),
				ntohs(cin.sin_port));

		/* handle the client		*/
		cli_hndl(cfd);

		/*
		 * cleanup and prepare
		 * for the next client
		 */
		close(cfd);
		bzero(&cin, sizeof(struct sockaddr_in));
		clen	= sizeof(struct sockaddr_in);
		cfd	= -1;
	}

	/* never reached			*/

	/* cleanup				*/
	cleanup(sfd);

	/* done; success			*/
	return EXIT_SUCCESS;

	/* error handling			*/
err:
	/* cleanup				*/
	cleanup(sfd);
	
	/* done; error				*/
	return EXIT_FAILURE;
}
