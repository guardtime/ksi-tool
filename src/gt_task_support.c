/**************************************************************************
 *
 * GUARDTIME CONFIDENTIAL
 *
 * Copyright (C) [2015] Guardtime, Inc
 * All Rights Reserved
 *
 * NOTICE:  All information contained herein is, and remains, the
 * property of Guardtime Inc and its suppliers, if any.
 * The intellectual and technical concepts contained herein are
 * proprietary to Guardtime Inc and its suppliers and may be
 * covered by U.S. and Foreign Patents and patents in process,
 * and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this
 * material is strictly forbidden unless prior written permission
 * is obtained from Guardtime Inc.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime Inc.
 */

#include <string.h>
#include <time.h>
#include <ksi/ksi.h>
#include <ksi/net.h>
#include <ksi/hashchain.h>
#include <ksi/pkitruststore.h>
#include <stdio.h>
#include "gt_task_support.h"
#include "param_set.h"
#include <ctype.h>
#include <ksi/tlv.h>
#include <ksi/tlv_template.h>
#include "ksitool_err.h"

#ifdef _WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#include <stdlib.h>
#else
#       include <limits.h>
#	include <sys/time.h>
#endif


/**
 * File object for logging
 */
static FILE *logFile = NULL;

static int getStreamFromPath(const char *fname, const char *mode, FILE **stream, bool *close) {
	int res;
	FILE *in = NULL;
	FILE *tmp = NULL;
	bool doClose = false;

	if (fname == NULL || mode == NULL || stream == NULL || close == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	if (strcmp(fname, "-") == 0) {
		tmp = strcmp(mode, "rb") == 0 ? stdin : stdout;
#ifdef _WIN32
		res = _setmode(_fileno(tmp),_O_BINARY);
		if (res == -1) {
			res = KT_UNABLE_TO_SET_STREAM_MODE;
			goto cleanup;
		}
#endif
	} else {
		in = fopen(fname, mode);
		if (in == NULL) {
			res = KT_IO_ERROR;
			goto cleanup;
		}

		doClose = true;
		tmp = in;
	}

	*stream = tmp;
	in = NULL;
	*close = doClose;

	res = KT_OK;

cleanup:

	if (in) fclose(in);
	return res;
}

static int ksitool_initLogger(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	paramSet *set = NULL;
	FILE *writeLogTo = NULL;
	bool log;
	bool closeLogStream = false;
	char *outLogfile = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	set = Task_getSet(task);
	log = paramSet_getStrValueByNameAt(set, "log",0, &outLogfile);

	/*Set logging*/
	if (log){
		res = getStreamFromPath(outLogfile, "w", &writeLogTo, &closeLogStream);
		if (res != KT_OK) {
			ERR_TRCKR_ADD(err, res, "Error:%s.", errToString(res));
			goto cleanup;
		}

		if (closeLogStream == true) logFile = writeLogTo;
		res = KSI_CTX_setLoggerCallback(ksi, KSI_LOG_StreamLogger, writeLogTo);
		ERR_CATCH_KSI(ksi, "Error: Unable to set logger callback.");
		res = KSI_CTX_setLogLevel(ksi, KSI_LOG_DEBUG);
		ERR_CATCH_KSI(ksi, "Error: Unable to set logger log level.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int ksitool_initNetworkProvider(Task *task, KSI_CTX *ksi, ERR_TRCKR *err){
	int res;
	paramSet *set = NULL;
	bool S, P, X, C, c, s, v, x, p, T, aggre;
	char *signingService_url = NULL;
	char *publicationsFile_url = NULL;
	char *verificationService_url = NULL;
	int networkConnectionTimeout = 0;
	int networkTransferTimeout = 0;
	char *user = NULL;
	char *pass = NULL;

	set = Task_getSet(task);
	S = paramSet_getHighestPriorityStrValueByName(set, "S", &signingService_url);
	X = paramSet_getHighestPriorityStrValueByName(set, "X", &verificationService_url);
	P = paramSet_getStrValueByNameAt(set, "P",0,&publicationsFile_url);

	C = paramSet_getIntValueByNameAt(set, "C", 0,&networkConnectionTimeout);
	c = paramSet_getIntValueByNameAt(set, "c", 0,&networkTransferTimeout);
	aggre = paramSet_isSetByName(set, "aggre");
	s = paramSet_isSetByName(set, "s");
	v = paramSet_isSetByName(set, "v");
	x = paramSet_isSetByName(set, "x");
	p = paramSet_isSetByName(set, "p");
	T = paramSet_isSetByName(set, "T");


	paramSet_getHighestPriorityStrValueByName(set, "user", &user);
	paramSet_getHighestPriorityStrValueByName(set, "pass", &pass);

	if (user == NULL) user = "anon";
	if (pass == NULL) pass = "anon";

	if (x || v || (p && T)) {
		if (verificationService_url == NULL){
			if ((x && v) || (p && T)) {
				ERR_TRCKR_ADD(err, res = KT_INVALID_CMD_PARAM, "Error: Extender url required.");
				goto cleanup;
			} else if (v) {
				print_warnings("Warning: verification may require extender url.\n");
			}
		}else {
			res = KSI_CTX_setExtender(ksi, verificationService_url, user, pass);
			ERR_CATCH_KSI(ksi, "Error: Unable set extender.");
		}
	}else if (s || aggre) {
		if (signingService_url == NULL) {
			ERR_TRCKR_ADD(err, res  = KT_INVALID_CMD_PARAM, "Error: Aggregator url required.");
			goto cleanup;
		}

		res = KSI_CTX_setAggregator(ksi, signingService_url, user, pass);
		ERR_CATCH_KSI(ksi, "Error: Unable set aggregator.");
	}

	if (P){
		res = KSI_CTX_setPublicationUrl(ksi, publicationsFile_url);
		ERR_CATCH_KSI(ksi, "Error: Unable set publication URL.");
	}

	if (C) {
		res = KSI_CTX_setConnectionTimeoutSeconds(ksi, networkConnectionTimeout);
		ERR_CATCH_KSI(ksi, "Error: Unable set connection timeout.");
	}

	if (c) {
		KSI_CTX_setTransferTimeoutSeconds(ksi, networkTransferTimeout);
		ERR_CATCH_KSI(ksi, "Error: Unable set transfer timeout.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int ksitool_initTrustStore(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res;
	paramSet *set = NULL;
	KSI_PKITruststore *refTrustStore = NULL;
	int i=0;
	bool V, W, E;
	char *lookupFile = NULL;
	char *lookupDir = NULL;
	char *magicEmail = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	/*Get parameter values*/
	set = Task_getSet(task);
	V = paramSet_isSetByName(set,"V");
	W = paramSet_getStrValueByNameAt(set, "W",0, &lookupDir);
	E = paramSet_getStrValueByNameAt(set, "E",0, &magicEmail);

	if(V || W) {
		res = KSI_CTX_getPKITruststore(ksi, &refTrustStore);
		ERR_CATCH_KSI(ksi, "Error: Unable to get PKI trust store.");
		if(V){
			while(paramSet_getStrValueByNameAt(set, "V", i++, &lookupFile)) {
				res = KSI_PKITruststore_addLookupFile(refTrustStore, lookupFile);
				ERR_CATCH_KSI(ksi, "Error: Unable to add cert to PKI trust store.");
			}
		}
		if(W){
			res = KSI_PKITruststore_addLookupDir(refTrustStore, lookupDir);
			ERR_CATCH_KSI(ksi, "Error: Unable to add lookup dir to PKI trust store.");
		}
	}

	if(E){
		res = KSI_CTX_setPublicationCertEmail(ksi, magicEmail);
		ERR_CATCH_KSI(ksi, "Error: Unable set publication certificate email.");
	}

	res = KT_OK;

cleanup:

	return res;
}

static int ksitool_initPublicationFile(Task *task, KSI_CTX *ksi, ERR_TRCKR *err) {
	int res = KSI_UNKNOWN_ERROR;
	paramSet *set = NULL;
	KSI_PublicationsFile *tmpPubFile = NULL;
	bool b;
	char *inPubFileName = NULL;

	if (task == NULL || ksi == NULL || err == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	/*Get parameter values*/
	set = Task_getSet(task);
	b = paramSet_getStrValueByNameAt(set, "b",0, &inPubFileName);

	if(b && (Task_getID(task) != downloadPublicationsFile && Task_getID(task) != verifyPublicationsFile)){
		res = loadPublicationFile(err, ksi, inPubFileName, &tmpPubFile);
		if (res != KT_OK) goto cleanup;

		res = KSI_CTX_setPublicationsFile(ksi, tmpPubFile);
		ERR_CATCH_KSI(ksi, "Error: Unable to configure publications file.");
		tmpPubFile = NULL;
	}

	res = KT_OK;

cleanup:

	KSI_PublicationsFile_free(tmpPubFile);

	return res;
}

int initTask(Task *task ,KSI_CTX **ksi, ERR_TRCKR **error) {
	int res;
	ERR_TRCKR *err = NULL;
	KSI_CTX *tmpKsi = NULL;

	if (task == NULL || ksi == NULL || error == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	/*Init error tracker*/
	err = ERR_TRCKR_new(print_errors);
	if (err == NULL) {
		res = KT_OUT_OF_MEMORY;
		print_errors("Error: Unable to initialize error tracker.");
		goto cleanup;
	}

	*error = err;

	/*Init KSI*/
	res = KSI_CTX_new(&tmpKsi);
	if (res != KSI_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to initialize KSI context.");
		goto cleanup;
	}

	res = ksitool_initLogger(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI logger.");
		goto cleanup;
	}

	res = ksitool_initNetworkProvider(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure network provider.");
		goto cleanup;
	}

	res = ksitool_initPublicationFile(task, tmpKsi, err);
	if (res != KT_OK) goto cleanup;


	res = ksitool_initTrustStore(task, tmpKsi, err);
	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to configure KSI trust store.");
		goto cleanup;
	}

	*ksi = tmpKsi;
	tmpKsi = NULL;
	res = KT_OK;


cleanup:

	KSI_CTX_free(tmpKsi);

	return res;
}

bool isPiping(paramSet *set) {
	bool o, log;
	int j;
	char *files[5] = {NULL, NULL, NULL, NULL, NULL};

	o = paramSet_getStrValueByNameAt(set, "o",0, &files[0]);
	log = paramSet_getStrValueByNameAt(set, "log",0, &files[1]);

	for (j = 0; j < 2; j++) {
		if (files[j] != NULL && strcmp(files[j], "-") == 0) {
			return true;
		}
	}

	return false;
}

void closeTask(KSI_CTX *ksi){
	if (ksi == NULL)
		return;
	if(logFile) fclose(logFile);
	KSI_CTX_free(ksi);
}



int getFilesHash(KSI_DataHasher *hsr, const char *fname, KSI_DataHash **hash){
	int res;
	FILE *readFrom = NULL;
	unsigned char buf[1024];
	size_t buf_len;
	bool close;
	KSI_DataHash *tmp = NULL;

	if(hsr == NULL || fname == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getStreamFromPath(fname, "rb", &readFrom, &close);
	if (res != KT_OK) goto cleanup;

	while (!feof(readFrom)) {
		buf_len = fread(buf, 1, sizeof (buf), readFrom);
		res = KSI_DataHasher_add(hsr, buf, buf_len);
		if (res != KSI_OK) goto cleanup;
	}

	res = KSI_DataHasher_close(hsr, &tmp);
	if (res != KSI_OK) goto cleanup;

	*hash = tmp;
	tmp = NULL;

	res = KT_OK;

cleanup:

	if (close == true && readFrom != NULL) fclose(readFrom);
	KSI_DataHash_free(tmp);

	return res;
}

static int loadKsiObj(KSI_CTX *ksi, const char *path, void **obj,
					int (*parse)(KSI_CTX *ksi, unsigned char *raw, unsigned raw_len, void **obj),
					void (*obj_free)()){
	int res;
	FILE *readFrom = NULL;
	bool close = false;
	unsigned char *buf = NULL;
	size_t buf_size = 0xffff;
	size_t buf_len = 0;
	void *tmp = NULL;

	if (ksi == NULL || path == NULL || obj == NULL || parse == NULL || free == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getStreamFromPath(path, "rb", &readFrom, &close);
	if (res != KT_OK) goto cleanup;

	buf = (unsigned char*)malloc(buf_size);
	if (buf == NULL) {
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	while (!feof(readFrom)) {
		if (buf_len + 1 >= buf_size) {
			buf_size += 0xffff;
			buf = realloc(buf, buf_size);
			if (buf == NULL) {
				res = KT_OUT_OF_MEMORY;
				goto cleanup;
			}
		}
		buf_len += fread(buf + buf_len, 1, buf_size - buf_len, readFrom);
	}

	if (buf_len > UINT_MAX) {
		res = KT_INDEX_OVF;
		goto cleanup;
	}

	res = parse(ksi, buf, (unsigned)buf_len, &tmp);
	if (res != KSI_OK || tmp == NULL) {
		goto cleanup;
	}

	*obj = tmp;
	tmp = NULL;
	res = KT_OK;


cleanup:

	if (close == true && readFrom != NULL) fclose(readFrom);
	free(buf);
	obj_free(tmp);

	return res;
}

int loadPublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_PublicationsFile **pubfile) {
	int res;

	if (err == NULL || ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = loadKsiObj(ksi, fname,
				(void**)pubfile,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_PublicationsFile_parse,
				(void (*)(void *))KSI_PublicationsFile_free);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: %s", errToString(res));
		ERR_TRCKR_ADD(err, res, "Error: Unable to load publication file from '%s'.", fname);
	}

	return res;
}

int loadSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, const char *fname, KSI_Signature **sig) {
	int res;

	if (ksi == NULL || fname == NULL || sig == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = loadKsiObj(ksi, fname,
				(void**)sig,
				(int (*)(KSI_CTX *, unsigned char*, unsigned, void**))KSI_Signature_parse,
				(void (*)(void *))KSI_Signature_free);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: %s", errToString(res));
		ERR_TRCKR_ADD(err, res, "Error: Unable to load signature file from '%s'.", fname);
	}
	return res;
}

static int saveKsiObj(KSI_CTX *ksi, void *obj,
							int (*serialize)(KSI_CTX *ksi, void *obj, unsigned char **raw, unsigned *raw_len),
							const char *path) {
	int res;
	bool doPipe = false;
	FILE *writeInto = NULL;
	bool close = false;
	unsigned char *raw = NULL;
	unsigned raw_len;
	size_t count;


	if (ksi == NULL || obj == NULL || serialize == NULL || path == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	doPipe = strcmp(path, "-") == 0 ? true : false;

	res = serialize(ksi, obj, &raw, &raw_len);
	if (res != KSI_OK) goto cleanup;

	res = getStreamFromPath(path, "wb", &writeInto, &close);
	if (res != KT_OK) goto cleanup;

	count = fwrite(raw, 1, raw_len, writeInto);
	if (count != raw_len) {
		res = KT_IO_ERROR;
		goto cleanup;
	}

	res = KT_OK;

cleanup:

	KSI_free(raw);
	if (close == true && writeInto != NULL) fclose(writeInto);

	return res;
}

int KSI_Signature_serialize_wrapper(KSI_CTX *ksi, KSI_Signature *sig, unsigned char **raw, unsigned *raw_len) {
	return KSI_Signature_serialize(sig, raw, raw_len);
}

int saveSignatureFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_Signature *sign, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || sign == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(ksi, sign,
				(int (*)(KSI_CTX *, void *, unsigned char **, unsigned *))KSI_Signature_serialize_wrapper,
				fname);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: %s", errToString(res));
		ERR_TRCKR_ADD(err, res, "Error: Unable to save signature file to '%s'.", fname);
	}

	return res;
}

int savePublicationFile(ERR_TRCKR *err, KSI_CTX *ksi, KSI_PublicationsFile *pubfile, const char *fname) {
	int res;

	if (ksi == NULL || fname == NULL || pubfile == NULL) {
		return KT_INVALID_ARGUMENT;
	}

	res = saveKsiObj(ksi, pubfile,
				(int (*)(KSI_CTX *, void *, unsigned char **, unsigned *))KSI_PublicationsFile_serialize,
				fname);

	if (res) {
		ERR_TRCKR_ADD(err, res, "Error: %s", errToString(res));
		ERR_TRCKR_ADD(err, res, "Error: Unable to save publication file to '%s'.", fname);
	}

	return res;
}

bool isSignatureExtended(const KSI_Signature *sig) {
	int res;
	KSI_PublicationRecord *pubRec = NULL;

	if (sig == NULL) return false;
	res = KSI_Signature_getPublicationRecord(sig, &pubRec);

	return pubRec == NULL ? false : true;
}

void printPublicationsFileReferences(const KSI_PublicationsFile *pubFile){
	int res = KSI_UNKNOWN_ERROR;
	KSI_LIST(KSI_PublicationRecord)* list_publicationRecord = NULL;
	KSI_PublicationRecord *publicationRecord = NULL;
	char buf[1024];
	int i, j;
	char *pLineBreak = NULL;
	char *pStart = NULL;

	if(pubFile == NULL) return;

	print_info("Publications file references:\n");

	res = KSI_PublicationsFile_getPublications(pubFile, &list_publicationRecord);
	if(res != KSI_OK) return;

	for (i = 0; i < KSI_PublicationRecordList_length(list_publicationRecord); i++) {
		int h=0;
		res = KSI_PublicationRecordList_elementAt(list_publicationRecord, i, &publicationRecord);
		if(res != KSI_OK) return;

		if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;

		pStart = buf;
		j=1;
		h=0;
		if(i) print_info("\n");
		while((pLineBreak = strchr(pStart, '\n')) != NULL){
			*pLineBreak = 0;
			if(h++ < 3)
				print_info("%s %s\n", "  ", pStart);
			else
				print_info("%s %2i) %s\n", "    ", j++, pStart);
			pStart = pLineBreak+1;
		}

		print_info("%s %2i) %s\n", "    ", j++, pStart);
	}
	print_info("\n");
	return;
}

void printSignaturePublicationReference(const KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	KSI_PublicationRecord *publicationRecord;
	char buf[1024];
	char *pLineBreak = NULL;
	char *pStart = buf;
	int i=1;
	int h=0;
	if(sig == NULL) return;

	print_info("Signatures publication references:\n");
	res = KSI_Signature_getPublicationRecord(sig, &publicationRecord);
	if(res != KSI_OK)return ;

	if(publicationRecord == NULL) {
		print_info("  (No publication records available)\n\n");
		return;
	}

	if(KSI_PublicationRecord_toString(publicationRecord, buf,sizeof(buf))== NULL) return;
	pStart = buf;

	while((pLineBreak = strchr(pStart, '\n')) != NULL){
		*pLineBreak = 0;

		if(h++<3)
			print_info("%s %s\n", "  ", pStart);
		else
			print_info("%s %2i) %s\n", "    ", i++, pStart);

		pStart = pLineBreak+1;
	}

	print_info("%s %2i) %s\n", "    ", i++, pStart);
	print_info("\n");

	return;
}

void printSignerIdentity(KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	char *signerIdentity = NULL;

	if(sig == NULL) goto cleanup;

	print_info("Signer identity: ");
	res = KSI_Signature_getSignerIdentity(sig, &signerIdentity);
	if(res != KSI_OK){
		print_info("Unable to get signer identity.\n");
		goto cleanup;
	}

	print_info("'%s'\n", signerIdentity == NULL || strlen(signerIdentity) == 0 ? "Unknown" : signerIdentity);
	print_info("\n");
cleanup:

	KSI_free(signerIdentity);
	return;
}

void printSignatureVerificationInfo(const KSI_Signature *sig){
	int res = KSI_UNKNOWN_ERROR;
	const KSI_VerificationResult *sigVerification = NULL;
	const KSI_VerificationStepResult *result = NULL;
	const char *desc;
	int i=0;

	if(sig == NULL){
		return;
	}

	print_info("Verification steps:\n");
	res = KSI_Signature_getVerificationResult((KSI_Signature*)sig, &sigVerification);
	if(res != KSI_OK){
		print_info("Unable to get verification steps\n\n");
		return;
	}

	if(sigVerification != NULL){
		for(i=0; i< KSI_VerificationResult_getStepResultCount(sigVerification); i++){
			res = KSI_VerificationResult_getStepResult(sigVerification, i, &result);
			if(res != KSI_OK){
				return;
			}
			print_info("  0x%03x:\t%s", KSI_VerificationStepResult_getStep(result), KSI_VerificationStepResult_isSuccess(result) ? "OK" : "FAIL");
			desc = KSI_VerificationStepResult_getDescription(result);
			if (desc && *desc) {
				print_info(" (%s)", desc);
			}
			print_info("\n");
		}
	}
	print_info("\n");
	return;
}

void printSignatureSigningTime(const KSI_Signature *sig) {
	int res;
	KSI_Integer *sigTime = NULL;
	unsigned long signingTime = 0;
	char date[1024];


	if (sig == NULL) {
		return;
	}


	res = KSI_Signature_getSigningTime(sig, &sigTime);
	if (res != KSI_OK) {
		return;
	}

	if (KSI_Integer_toDateString(sigTime, date, sizeof(date)) != date) {
		return;
	}

	signingTime = (unsigned long)KSI_Integer_getUInt64(sigTime);

	print_info("Signing time:\n"
			"UTC seconds:%i\n"
			"Date %s\n", signingTime, date);

	print_info("\n");
	return;
}

void printPublicationsFileCertificates(const KSI_PublicationsFile *pubfile){
	KSI_CertificateRecordList *certReclist = NULL;
	KSI_CertificateRecord *certRec = NULL;
	KSI_PKICertificate *cert = NULL;
	char buf[1024];
	int i=0;
	int res = 0;

	if(pubfile == NULL) goto cleanup;
	print_info("Publications file certificates::\n");

	res = KSI_PublicationsFile_getCertificates(pubfile, &certReclist);
	if(res != KSI_OK || certReclist == NULL) goto cleanup;

	for(i=0; i<KSI_CertificateRecordList_length(certReclist); i++){
		res = KSI_CertificateRecordList_elementAt(certReclist, i, &certRec);
		if(res != KSI_OK || certRec == NULL) goto cleanup;

		res = KSI_CertificateRecord_getCert(certRec, &cert);
		if(res != KSI_OK || cert == NULL) goto cleanup;

		if(KSI_PKICertificate_toString(cert, buf, sizeof(buf)) != NULL)
			print_info("  %2i)  %s\n",i, buf);
		else
			print_info("  %2i)  null\n",i);
	}

cleanup:

	return;
	}

KSI_IMPORT_TLV_TEMPLATE(KSI_Signature);

void printSignatureStructure(KSI_CTX *ksi, const KSI_Signature *sig) {
	int res;
	KSI_TLV *baseTlv = NULL;
	unsigned char *tmp = NULL;
	unsigned tmp_len;
	char buf[0x6fff];

	res = KSI_Signature_serialize((KSI_Signature*)sig, &tmp, &tmp_len);
	if (res != KSI_OK) goto cleanup;

	res = KSI_TLV_new(ksi, KSI_TLV_PAYLOAD_TLV, 0x800, 0, 0, &baseTlv);
	if (res != KSI_OK) goto cleanup;

	res = KSI_TlvTemplate_construct(ksi, baseTlv, sig, KSI_Signature_template);
	if (res != KSI_OK) goto cleanup;

	if (KSI_TLV_toString(baseTlv, buf, sizeof(buf)) == NULL) return;
	print_info("Signature's TLV structure:\n%s\n", buf);

cleanup:

	KSI_free(tmp);
	KSI_TLV_free(baseTlv);

	return;
}

// helpers for hex decoding
static int x(char c){
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	abort(); // isxdigit lies.
	return -1; // makes compiler happy
}

static int xx(char c1, char c2){
	if(!isxdigit(c1) || !isxdigit(c2))
		return -1;
	return x(c1) * 16 + x(c2);
}

/**
 * Converts a string into binary array.
 *
 * @param[in] ksi Pointer to KSI KSI_CTX object.
 * @param[in] hexin Pointer to string for conversion.
 * @param[out] binout Pointer to receiving pointer to binary array.
 * @param[out] lenout Pointer to binary array length.
 */
static int getBinaryFromHexString(const char *hexin, unsigned char **binout, size_t *lenout){
	int res;
	size_t len;
	unsigned char *tmp = NULL;
	size_t arraySize;
	int i, j;

	if (hexin == NULL || binout == NULL || lenout == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	len = strlen(hexin);
	arraySize = len / 2;

	if (len%2 != 0) {
		res = KT_HASH_LENGTH_IS_NOT_EVEN;
		goto cleanup;
	}

	tmp = KSI_calloc(arraySize, sizeof(unsigned char));
	if(tmp == NULL){
		res = KT_OUT_OF_MEMORY;
		goto cleanup;
	}

	for (i = 0, j = 0; i < arraySize; i++, j += 2){
		int value = xx(hexin[j], hexin[j+1]);
		if(value == -1){
			res = KT_INVALID_HEX_CHAR;
			goto cleanup;
		}

		tmp[i] = (unsigned char)value;
	}

	*lenout = arraySize;
	*binout = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	KSI_free(tmp);

	return res;
}

static int getHashAndAlgStrings(const char *instrn, char **strnAlgName, char **strnHash){
	int res ;
	char *colon = NULL;
	size_t algLen = 0;
	size_t hahsLen = 0;
	char *temp_strnAlg = NULL;
	char *temp_strnHash = NULL;
	*strnAlgName = NULL;
	*strnHash = NULL;


	if(instrn == NULL || strnAlgName == NULL || strnHash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	colon = strchr(instrn, ':');
	if (colon != NULL) {
		algLen = (colon - instrn) / sizeof (char);
		hahsLen = strlen(instrn) - algLen - 1;

		temp_strnAlg = calloc(algLen + 1, sizeof (char));
		if (temp_strnAlg == NULL) {
			res = KT_OUT_OF_MEMORY;
			goto cleanup;
		}

		temp_strnHash = calloc(hahsLen + 1, sizeof (char));
		if (temp_strnHash == NULL) {
			res = KT_OUT_OF_MEMORY;
			goto cleanup;
		}

		memcpy(temp_strnAlg, instrn, algLen);
		temp_strnAlg[algLen] = 0;
		memcpy(temp_strnHash, colon + 1, hahsLen);
		temp_strnHash[hahsLen] = 0;
	} else {
		res = KT_INVALID_INPUT_FORMAT;
		goto cleanup;
	}

	*strnAlgName = temp_strnAlg;
	*strnHash = temp_strnHash;
	temp_strnAlg = NULL;
	temp_strnHash = NULL;

	res = KT_OK;

cleanup:

	free(temp_strnAlg);
	free(temp_strnHash);

	return res;
}

int getHashFromCommandLine(const char *imprint, KSI_CTX *ksi, ERR_TRCKR *err, KSI_DataHash **hash){
	int res;
	unsigned char *data = NULL;
	size_t len;
	int hasAlg;
	char *strAlg = NULL;
	char *strHash = NULL;
	KSI_DataHash *tmp = NULL;

	if (imprint == NULL || ksi == NULL || hash == NULL) {
		res = KT_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = getHashAndAlgStrings(imprint, &strAlg, &strHash);
	if (res != KT_OK) goto cleanup;

	res = getBinaryFromHexString(strHash, &data, &len);
	if (res != KT_OK) goto cleanup;

	hasAlg = KSI_getHashAlgorithmByName(strAlg);
	if (hasAlg == -1) {
		res = KT_UNKNOWN_HASH_ALG;
		goto cleanup;
	}

	res = KSI_DataHash_fromDigest(ksi, hasAlg, data, (unsigned int)len, &tmp);
	if (res != KSI_OK) goto cleanup;

	*hash = tmp;
	tmp = NULL;
	res = KT_OK;

cleanup:

	free(strAlg);
	free(strHash);
	free(data);
	KSI_DataHash_free(tmp);

	if (res != KT_OK) {
		ERR_TRCKR_ADD(err, res, "Error: Unable to get hash from command-line");
		ERR_TRCKR_ADD(err, res, "Error: ", errToString(res));
	}

	return res;
}

static unsigned int elapsed_time_ms;

unsigned int measureLastCall(void){
#ifdef _WIN32
    static LARGE_INTEGER thisCall;
    static LARGE_INTEGER lastCall;
    LARGE_INTEGER frequency;        // ticks per second

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&thisCall);

    elapsed_time_ms = (unsigned)((thisCall.QuadPart - lastCall.QuadPart) * 1000.0 / frequency.QuadPart);
#else
    static struct timeval thisCall = {0, 0};
    static struct timeval lastCall = {0, 0};

    gettimeofday(&thisCall, NULL);

    elapsed_time_ms = (unsigned)((thisCall.tv_sec - lastCall.tv_sec) * 1000.0 + (thisCall.tv_usec - lastCall.tv_usec) / 1000.0);
#endif

    lastCall = thisCall;
    return elapsed_time_ms;
}

unsigned int measuredTime(void){
	return elapsed_time_ms;
}

char* str_measuredTime(void){
	static char buf[32];
	snprintf(buf,32,"(%i ms)", elapsed_time_ms);
	return buf;
}


int ksi_error_wrapper(ERR_TRCKR *err, KSI_CTX *ksi, int res, const char *file, unsigned line, char *msg, ...) {
	va_list va;
	int extError = KSI_UNKNOWN_ERROR;
	int baseError = KSI_UNKNOWN_ERROR;
	char buf[1024];
	char buf2[2048];

	if (err == NULL || msg == NULL || file == NULL) return KT_INVALID_ARGUMENT;

	if(res != KSI_OK) {
		KSI_LOG_logCtxError(ksi, KSI_LOG_DEBUG);
		KSI_ERR_getBaseErrorMessage(ksi, buf, sizeof(buf), &baseError, &extError);

		if(strlen(buf) > 0){
			if(extError)
				snprintf(buf2, sizeof(buf2), "Error: %s (KSI:0x%x, EXT:0x%x)", buf, baseError, extError);
			else
				snprintf(buf2, sizeof(buf2), "Error: %s (KSI:0x%x)", buf, baseError);
		}
		else {
			snprintf(buf2, sizeof(buf2), "Error: %s (KSI:0x%x)", KSI_getErrorString(res), res);
		}

		ERR_TRCKR_add(err, res, file, line, buf2);
		if (msg != NULL) {
			va_start(va, msg);
			vsnprintf(buf2, sizeof(buf2), msg, va);
			va_end(va);
			ERR_TRCKR_add(err, res, file, line, buf2);
		}
	}

	return res;
}
