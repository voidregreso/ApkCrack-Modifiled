#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "zhao_apkcrack_ResUtils_ELF_Elf.h"
#include "zhao_apkcrack_Utils_FileUtil.h"
#include <ZipAlign.h>

#define DEX_OPT_MAGIC   "dey\n"
#define DEX_MAGIC       "dex\n"
struct odex_header {
	uint8_t magic[8];
	uint32_t dex_off;
	uint32_t dex_len;
};
struct dex_header {
	uint8_t magic[8];
	uint8_t padding[24];
	uint32_t file_len;
};

long ELFHash(const char *a1) {
	long v1 = 0;
	while ( *a1 ) {
		v1 = *(unsigned long*)a1++ + 16 * v1;
		if ( v1 >> 28 << 28 )
			v1 = (v1 ^ 16 * (v1 >> 28) & 0xFF) & ~(v1 >> 28 << 28);
	}
	return 2 * v1 >> 1;
}

JNIEXPORT jlong JNICALL Java_zhao_apkcrack_ResUtils_ELF_Elf_ELFHash
(JNIEnv *env, jobject obj, jstring str) {
	return (jlong)ELFHash(env->GetStringUTFChars(str,JNI_FALSE));
}

JNIEXPORT jstring JNICALL Java_zhao_apkcrack_Utils_FileUtil_deodex
(JNIEnv *env, jclass cls, jstring in, jstring out) {
	int ret = -1;
	int odexFD = 0, dexFD = 0;
	size_t odexSize = 0, dexSize = 0;
	if((odexFD = open(env->GetStringUTFChars(in,JNI_FALSE), O_RDONLY)) == -1) {
		char ss[2048];
		sprintf(ss,"Cannot open input odex %s",env->GetStringUTFChars(in,JNI_FALSE));
		return env->NewStringUTF(ss);
	}
	if((dexFD = open(env->GetStringUTFChars(out,JNI_FALSE), O_RDWR|O_CREAT, 00660)) == -1) {
		char ss[2048];
		sprintf(ss,"Cannot open output dex %s",env->GetStringUTFChars(out,JNI_FALSE));
		close(odexFD);
		return env->NewStringUTF(ss);
	}
	struct stat Stat;
	if(fstat(odexFD, &Stat)) {
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("fstat odex error");
	}
	odexSize = Stat.st_size;
	uint8_t *odexBuf = NULL, *dexBuf = NULL;
	if((odexBuf = (uint8_t*)mmap(NULL, odexSize, PROT_READ, MAP_SHARED, odexFD, 0)) == MAP_FAILED) {
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("mmap odex error");
	}
	struct odex_header *odexH = (struct odex_header *)odexBuf;
	struct dex_header *dexH = (struct dex_header *)(odexBuf + odexH->dex_off);
	if(memcmp(odexH->magic, DEX_OPT_MAGIC, 4)) {
		munmap(odexBuf, odexSize);
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("odex bad magic error\n");
	}
	if(memcmp(dexH->magic, DEX_MAGIC, 4)) {
		munmap(odexBuf, odexSize);
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("dex bad magic error\n");
	}
	if(odexH->dex_len != dexH->file_len || (odexH->dex_len + 40) > odexSize) {
		munmap(odexBuf, odexSize);
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("dex bad file len error\n");
	}
	dexSize = odexH->dex_len;
	if(ftruncate(dexFD, dexSize)) {
		munmap(odexBuf, odexSize);
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("ftruncate dex error");
	}
	if((dexBuf = (uint8_t*)mmap(NULL, dexSize, PROT_READ|PROT_WRITE, MAP_SHARED, dexFD, 0)) == MAP_FAILED) {
		munmap(odexBuf, odexSize);
		close(odexFD);
		close(dexFD);
		return env->NewStringUTF("mmap dex error");
	}
	memcpy(dexBuf, odexBuf + odexH->dex_off, dexSize);
	munmap(dexBuf, dexSize);
	munmap(odexBuf, odexSize);
	close(odexFD);
	close(dexFD);
	return env->NewStringUTF("Deodex Success!");
}

JNIEXPORT jboolean JNICALL Java_zhao_apkcrack_Utils_FileUtil_testExecute
(JNIEnv *env, jclass cls, jstring jstr) {
	return (access(env->GetStringUTFChars(jstr,JNI_FALSE),1)<=0);
}

inline void write_to_file(unsigned char *data, const char *filepath, int size, int file_count) {
	char filename[1024];
	sprintf(filename, "%s%02d.dex", filepath, file_count);
//	printf("Writing %d bytes to %s\n", size, filename);
	FILE *fp = fopen(filename, "wb");
	fwrite(data, 1, size, fp);
	fclose(fp);
}

JNIEXPORT jstring JNICALL Java_zhao_apkcrack_Utils_FileUtil_oat2dex
(JNIEnv *env, jclass cls, jstring in, jstring out) {
	FILE *infp = fopen(env->GetStringUTFChars(in,JNI_FALSE), "rb");
	if (infp == NULL) {
		char ss[1024];
		sprintf(ss,"Can't open %s\n", env->GetStringUTFChars(in,JNI_FALSE));
		return env->NewStringUTF(ss);
	}

	fseek(infp, 0, SEEK_END);
	int insize = ftell(infp);
	fseek(infp, 0, SEEK_SET);
	unsigned char *indata = (unsigned char*)malloc(insize);
	fread(indata, 1, insize, infp);
	fclose(infp);

	int file_count = 0;
	int ptr;
	for (ptr = 0; ptr < insize; ptr ++) {
		if (memcmp(indata+ptr, "dex\n035", 8) != 0)
			continue;
		unsigned int dexsize = *(unsigned int *)(indata+ptr+32); // the 'file_size_' field in the header
		if (ptr + dexsize > insize)
			continue;
		write_to_file(indata+ptr, env->GetStringUTFChars(out,JNI_FALSE), dexsize, ++file_count);
	}
	return env->NewStringUTF("oat2dex Success!");
}

JNIEXPORT jstring JNICALL Java_zhao_apkcrack_Utils_FileUtil_zipalign
(JNIEnv *env, jclass cls, jstring in, jstring out, jint mode) {
	const char *s1 = env->GetStringUTFChars(in, NULL);
    const char *s2 = env->GetStringUTFChars(out, NULL);
    if(zipalign(s1, s2, mode, 0) == 1) return env->NewStringUTF("ZipAlign success!");
    else return env->NewStringUTF("Failed to zipalign!");
}
