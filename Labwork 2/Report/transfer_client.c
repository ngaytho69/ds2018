#include "transfer.h"
#include <stdio.h>
#include <stdlib.h>
void transfer_1(char *host, char *filename)
{
	int  *result;
	file  transfer_arg;
	FILE *f;
	long int total = 0;

	/*Create client */
	CLIENT *clt;
	clt = clnt_create (host, TRANSFER, TRANSFER_1, "tcp");
	/* Error in connecting with server */
	if (clt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}

	f = fopen(filename, "rb");

	if(f == NULL) {
		printf("File not found.\n");
		exit(1);
	}

	printf("Sending file %s.\n", filename);

	strcpy(transfer_arg.name, filename);

	while(1) {
		transfer_arg.nbytes = fread(transfer_arg.data, 1, MAXLEN, f);
		total += transfer_arg.nbytes;	

		result = transf_1(&transfer_arg, clt);

		if (result == (int *) NULL) {
			clnt_perror (clt, "Failed to call");
		}

		/* Succesfully got data from the file*/
		if(transfer_arg.nbytes < MAXLEN) {
			printf("\n Upload file successfully. \n");
			break;
		}
	}

	clnt_destroy (clt);
	fclose(f);

}


int main (int argc, char *argv[])
{
	/*Get host */
	char *host;
	host = argv[1];
	/* Get name of the file being transfered */
	char *filename;
	filename = argv[2];

	if (argc < 3) {
		printf ("usage: %s <server_host> <file>\n", argv[0]);
		exit (1);
	}
	transfer_1 (host, filename);
    exit (0);
}
