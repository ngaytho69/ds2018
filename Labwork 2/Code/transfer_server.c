#include "transfer.h"
#include <stdio.h>
#include <stdlib.h>

int * transf_1_svc(file *argp, struct svc_req *rqstp)
{
	static int  result;	

	char opened_file[MAXLEN];
	FILE *f;
	long long int total = 0;

	/* Name of the new file was added "received_" at first */
	static char tempName[MAXLEN];
	strcpy(tempName, "received_");
	strcat(tempName, argp->name);
	strcpy(argp->name, tempName);	

	total += argp->nbytes;
	strcpy(opened_file, argp->name);
	f = fopen(argp->name, "a");
	
	fwrite(argp->data, 1, argp->nbytes, f);

	if (argp->nbytes < MAXLEN) {
		printf("\n Finished receiving file : %s.\n", argp->name);
		total = 0;
		fclose(f);
	}
	return &result;
}
