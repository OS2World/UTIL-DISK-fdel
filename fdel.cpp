#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSMISC
#include <os2.h>

void showUsage(void) {
	printf("fdel - force delete\n");
	printf("usage: fdel filespec\n");
	printf("Note: the files deleted cannot be retrieved with undelete.\n");
}

int confirmAllDelete(const char *fspec) {
	char s[20];
	do {
		printf("%s, are you VERY sure? (Y/N)",fspec);
		scanf("%s",s);
	} while(s[0]!='Y' &&s[0]!='y' &&s[0]!='N' &&s[0]!='n');
	return s[0]=='Y' || s[0]=='y';
}

#pragma off (unreferenced)
void displayError(const char *func, APIRET rc) {
#pragma on (unreferenced)
#ifndef NDEBUG
	fprintf(stderr,"%s: rc=%ld\n",func,rc);
#endif
	APIRET trc;
	ULONG msglen;
	char errmsg[256];
	trc = DosGetMessage((PCHAR)NULL, 0, (PCHAR)errmsg, 256, (ULONG)rc, (PSZ)"oso001.msg", (PULONG)&msglen);
	if(trc!=0)
		fprintf(stderr,"DosGetMessage() error. rc=%u (original rc=%u)",trc,rc);
	else
		fprintf(stderr,"%s\n",errmsg);
	exit((int)rc);
}

void deleteAFile(const char *drive, const char *dir, const char *fname) {
	char tdrive[_MAX_DRIVE],tdir[_MAX_DIR],name[_MAX_FNAME],ext[_MAX_EXT];
	char fullname[_MAX_PATH];
	_splitpath(fname,tdrive,tdir,name,ext);
	_makepath(fullname,drive,dir,name,ext);
#ifdef DEBUG
	printf("Would delete '%s'\n",fullname); fflush(stdout);
#else
        APIRET rc;
        rc = DosForceDelete(fullname);
        if(rc!=0) displayError("DosForceDelete", rc);
#endif
}

int main(int argc, char *argv[]) {
	if(argc!=2) {
		showUsage();
		return 1;
	} else {
		const char *fspec=argv[1];
		char drive[_MAX_DRIVE],dir[_MAX_DIR],name[_MAX_FNAME],ext[_MAX_EXT];
		_splitpath(fspec,drive,dir,name,ext);
		if((strcmp(name,"*")==0 || strcmp(name,"")==0) &&
		   (strcmp(ext,".*")==0 || strcmp(ext,".")==0))
		{
			if(!confirmAllDelete(fspec)) return 0;
		}

		HDIR hdir = 0x0001;
		APIRET rc;
		FILEFINDBUF3 ffbuf;
		ULONG count;

		count=1;
		rc = DosFindFirst((PSZ)fspec, &hdir, FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY, (PVOID)&ffbuf, sizeof(ffbuf), (PULONG)&count, FIL_STANDARD);
		if(rc!=0)
			if(rc!=ERROR_NO_MORE_FILES)
				displayError("DosFindFirst",rc);
			else
				displayError("arg",ERROR_FILE_NOT_FOUND);
		while(rc==0) {
			deleteAFile(drive,dir,ffbuf.achName);
			rc = DosFindNext(hdir, (PVOID)&ffbuf, sizeof(ffbuf), (PULONG)&count);
		}
		if(rc!=ERROR_NO_MORE_FILES)
			displayError("DosFindNext",rc);

		rc = DosFindClose(hdir);
		if(rc!=0) displayError("DosFindClose",rc);
	};
	return 0;
}
