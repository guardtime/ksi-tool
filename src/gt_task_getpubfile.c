#include "gt_task_support.h"
#include "try-catch.h"



bool GT_getPublicationsFileTask(GT_CmdParameters *cmdparam){
	KSI_CTX *ksi = NULL;
	KSI_PublicationsFile *publicationsFile = NULL;
	FILE *out = NULL;
	bool state = true;
	unsigned int count;
	char *raw = NULL;
	int raw_len = 0;

	/*Initalization of KSI */
	resetExeptionHandler();
	try
		CODE{
			initTask_throws(cmdparam ,&ksi);

			printf("Downloading publications file...");
			MEASURE_TIME(KSI_receivePublicationsFile_throws(ksi, &publicationsFile));
			printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

			printf("Verifying publications file...");
			MEASURE_TIME(KSI_verifyPublicationsFile_throws(ksi, publicationsFile));
			printf("ok. %s\n",cmdparam->t ? str_measuredTime() : "");

			KSI_PublicationsFile_serialize(ksi, publicationsFile, &raw, &raw_len);
			out = fopen(cmdparam->outPubFileName, "wb");
			if(out == NULL) THROW_MSG(IO_EXEPTION, "Unable to ope publications file '%s' for writing.\n", cmdparam->outPubFileName);

			count = fwrite(raw, 1, raw_len, out);
			if(count != raw_len) THROW_MSG(IO_EXEPTION, "Error: Unable to write publications file '%s'.\n", cmdparam->outPubFileName);
			printf("Publications file '%s' saved.\n", cmdparam->outPubFileName);
		}
		CATCH(KSI_EXEPTION){
				printf("failed.\n");
				printErrorLocations();
				exeptionSolved();
				state = false;
				goto cleanup;
		}
		CATCH(IO_EXEPTION){
				fprintf(stderr , _EXP.tmp);
				printErrorLocations();
				state = false;
				goto cleanup;
		}
	end_try

cleanup:
	if (out != NULL) fclose(out);
	KSI_CTX_free(ksi);
	return state;
}
