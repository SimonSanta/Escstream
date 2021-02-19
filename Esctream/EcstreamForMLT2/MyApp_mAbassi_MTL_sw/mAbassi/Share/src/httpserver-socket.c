/* ------------------------------------------------------------------------------------------------ */
/* FILE :		httpserver_socket.c																	*/
/*																									*/
/* CONTENTS :																						*/
/*				Code for the lwIP webserver with BSD sockets.										*/
/*				This file contains all the web services management.									*/
/*																									*/
/*																									*/
/* Copyright (c) 2013-2019, Code-Time Technologies Inc. All rights reserved.						*/
/*																									*/
/* Code-Time Technologies retains all right, title, and interest in and to this work				*/
/*																									*/
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS							*/
/* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF										*/
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL							*/
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR								*/
/* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,							*/
/* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR							*/
/* OTHER DEALINGS IN THE SOFTWARE.																	*/
/*																									*/
/*																									*/
/*	$Revision: 1.32 $																				*/
/*	$Date: 2019/01/10 18:07:18 $																	*/
/*																									*/
/* ------------------------------------------------------------------------------------------------ */

#include "WebApp.h"
#if defined(__GNUC__) && ((OS_SYS_CALL) != 0)
  #undef close										/* There is a conflict between these thatr are	*/
  #undef read										/* defined in SysCall and sockets.h	that also	*/
  #undef write										/* defines them differently						*/
#endif
#include "lwip/sockets.h"

/* ------------------------------------------------------------------------------------------------ */

enum VerbTypes {V_GET,   V_HEAD,    V_POST,    V_PUT,   V_DELETE,
                V_TRACE, V_OPTIONS, V_CONNECT, V_PATCH, V_UNKNOWN_};

struct Methods {									/* Association of text to verbs					*/
	char          *Text;
	enum VerbTypes Verb;
};

/* ------------------------------------------------------------------------------------------------ */

static void WebserverProcess(int conn); 
static void WebserverTask(void *Arg);

/* ------------------------------------------------------------------------------------------------ */
													/* Out larger than in for SSI processing		*/
#ifndef HTTP_BUF_SIZE
  static char g_PageInBuf[32768-2048];				/* Buffer holding input data when manipulated	*/
  static char g_PageOutBuf[32768];					/* Buffer holding output data when manipulated	*/
#else
  static char g_PageInBuf[HTTP_BUF_SIZE-2048];
  static char g_PageOutBuf[HTTP_BUF_SIZE];
#endif
static char g_RqstBuf[8192];						/* Buffer holding the request string			*/

static struct Methods g_MyVerbs[] = {				/* Association of text to verbs					*/
	{"GET ",     V_GET},
	{"HEAD ",    V_HEAD},
	{"POST ",    V_POST},
	{"PUT ",     V_PUT},
	{"DELETE ",  V_DELETE},
	{"TRACE ",   V_TRACE},
	{"OPTIONS ", V_OPTIONS},
	{"CONNECT ", V_CONNECT},
	{"PATCH ",   V_PATCH}
};

/* ------------------------------------------------------------------------------------------------ */
/* This one needs a bit of explanation.																*/
/* Normally, one would use																			*/
/* 		FATfs g_FileSys;																			*/
/* And this should work without issues. But there is no guarantee the field buf[_MAX_SS] in FATS	*/
/* will be aligned on 32 bytes (cache line alignment), so when the SD/MMC driver is used it will	*/
/* flush the data after the end of buf[_MAX_SS] when non-aligned. This should not be a problem		*/
/* but it can be depending on what is that extra data. One bad case is that data is involved in a 	*/
/* DMA transfer dealt by another task.																*/
/* The padding fills the extra data that will be flushed, safeguarding other data.					*/
/* The same issue exists for the data preceding the buffer as the smallest number of bytes in the	*/
/* the FATFS data structure that preceded the buffer is 10 bytes, so a padding of 22 is needed.		*/
/* Instead, we align the whole data structure on 32 bytes, so the before padding is not needed		*/

#if (((OS_DEMO) == 13) || ((OS_DEMO) == 113) || ((OS_DEMO) == -13) || ((OS_DEMO) == -113))
 #if ((OX_CACHE_LSIZE) != 1)
  static struct {
	FATFS   FileSys;
	uint8_t Pad2[OX_CACHE_LSIZE];
  } gg_FileSys __attribute__ ((aligned (OX_CACHE_LSIZE)));
  #define g_FileSys (gg_FileSys.FileSys)
 #else
  static FATFS g_FileSys;  
 #endif
#endif

/* ------------------------------------------------------------------------------------------------ */
/* Initialization of the HTTP server																*/
/*																									*/
/* - Create and resume the HTTP server task															*/
/* ------------------------------------------------------------------------------------------------ */

void http_server_init()
{

	sys_thread_new("HTTP server", WebserverTask, NULL, WEBS_STACKSIZE, WEBS_PRIO);

	return;
}

/* ------------------------------------------------------------------------------------------------ */
/* HTTP server task																					*/
/*																									*/
/* - Create a new TCP connection																	*/
/* - Init the CGIs and SSIs																			*/
/* - Bind the network connection to port 80															*/
/* - Start listening on the connection																*/
/* - forever loop																					*/
/*   - Block until a new connection with the client is established									*/
/*   - Process the client request																	*/
/*   - Delete the new connection																	*/
/* ------------------------------------------------------------------------------------------------ */

static void WebserverTask(void *Arg)
{
int ii;
int jj;
int newconn;
int size;
int sock;
struct sockaddr_in address;
struct sockaddr_in remotehost;


	Arg = Arg;										/* To remove compiler warning					*/

	WebServerInit();

  #if (((OS_DEMO) == 13) || ((OS_DEMO) == 113) || ((OS_DEMO) == -13) || ((OS_DEMO) == -113))
	memset(&g_FileSys, 0, sizeof(g_FileSys));
  #endif

	jj = 0;											/* Make sure the SD/MMC card can be mounted		*/
	do {											/* If not, retry 10 second later over and over	*/
		ii = F_MNT();
		MTXLOCK_STDIO();
		if (ii != 0) {
			puts("**** ERROR: Failed to mount the SD/MMC card");
			puts("            Make sure the SD/MMC card is inserted");
			puts("            Retrying in 5 s");
			MTXUNLOCK_STDIO();
			TSKsleep(5*OS_TICK_PER_SEC);
			jj = 1;
		}
		else {
			if (jj != 0) {
				puts("\n*** OK: SD/MMC card mounted\n");
			}
			MTXUNLOCK_STDIO();
		}
	} while(ii != 0);

	MTXLOCK_STDIO();
	puts("The file system is ready");
	MTXUNLOCK_STDIO();

  #ifndef osCMSIS
   #if ((OX_EVENTS) != 0)
	EVTset(TSKgetID("A&E"), 2);						/* Report to Adam & Eve File System is ready	*/
   #else
	MTXlock(G_OSmutex, -1);
	G_WebEvents |= 2;
	MTXunlock(G_OSmutex);
   #endif
  #else
	extern osThreadId G_TaskMainID;
	osSignalSet(G_TaskMainID, 2);
  #endif

	sock = socket(AF_INET, SOCK_STREAM, 0);			/* Create a new TCP socket						*/
	if (sock >= 0) {
		address.sin_family = AF_INET;
		address.sin_port = htons(80);
		address.sin_addr.s_addr = INADDR_ANY;

		size = bind(sock, (struct sockaddr *)&address, sizeof (address));
		if (size >= 0) {							/* use size as a temp							*/
			listen(sock, 5);						/* Listen for incoming, TCP backlog set to 5	*/
			size = sizeof(remotehost);
			while(1) {								/* And we go on forever							*/
				newconn = accept(sock, (struct sockaddr *)&remotehost, (socklen_t *)&size);
				WebserverProcess(newconn);
			}
		}
		else {
			MTXLOCK_STDIO();
			printf("Cannot bind the connection to port 80");
			MTXUNLOCK_STDIO();
		}
	}
	else {
		MTXLOCK_STDIO();
		printf("Cannot create the socket");
		MTXUNLOCK_STDIO();
	}

	for (;;) {
		TSKselfSusp();
	}
}

/* ------------------------------------------------------------------------------------------------ */
/* Process a client request																			*/
/*																									*/
/* - Get the request																				*/
/* - Find the request verb/method																	*/
/* - Per Verb processing																			*/
/*																									*/
/*   - GET verb:																					*/
/*     - Open the requested file																	*/
/*     - if file opening OK																			*/
/*       - if file extension .stm or .shtm or .shtml												*/
/*         - Copy the file contents to a local buffer												*/
/*         - Perform the SSI substitution															*/
/*         - Request to output the buffer															*/
/*       - else																						*/
/*         - Request to output the file																*/
/*     - if file opening error																		*/
/*       - Check and process CGIs																	*/
/*       - Open the file returned by CGI processing													*/
/*       - if file open OK																			*/
/*         - Request to output the file																*/
/*     - if no valid output																			*/
/*       - Open the 404 error page file																*/
/*       - if file open OK																			*/
/*         - Request to output the file																*/
/*       - else																						*/
/*         - Try to mount the file system															*/
/*     - If request to output file																	*/
/*       - Copy file to output																		*/
/*     - If request to output buffer																*/
/*       - Copy buffer to output																	*/
/*     - Close file																					*/
/*																									*/
/*   - Other verbs:																					*/
/*     - print error message																		*/
/*																									*/
/* - Close connection																				*/
/* ------------------------------------------------------------------------------------------------ */

static void WebserverProcess(int conn) 
{
int             CopyFile;
FILE_DSC_t      Fdsc;								/* Descriptor of the file requested by client	*/
int             FisOpen;							/* If the file to send back is open				*/
int             Fsize;								/* Number of bytes read from the file			*/
int             ii;									/* General purpose								*/
int             Len;								/* Length of the request						*/
int             PageLen;							/* Length of the page to send back to client	*/
char           *uri;
char            URIchar;							/* Character that was replaced by '\0'			*/
char           *URIend;								/* Index where the '\0' was inserted			*/
enum VerbTypes  VerbRqst;							/* Verb / method of the client request			*/

	F_INIT_FDSC(Fdsc);								/* To remove compiler warning					*/

	URIchar = '\0';
	uri     = &URIchar;
	Len = read(conn, &g_RqstBuf[0], sizeof(g_RqstBuf)-1);	/* Read the data from the port			*/
	if (Len >= 0) {									/* Got a valid request							*/
		g_RqstBuf[Len] = '\0';						/* String terminate the input					*/
		VerbRqst      = V_UNKNOWN_;					/* Assume we can't understand the verb			*/
		for(ii=0 ; ii<(sizeof(g_MyVerbs)/sizeof(g_MyVerbs[0])) ; ii++) {
			if (0 == strncmp(&g_RqstBuf[0], g_MyVerbs[ii].Text, strlen(g_MyVerbs[ii].Text))) {
				uri = strchr(&g_RqstBuf[0], ' ');	/* Isolate the resource name					*/
				if (uri != (char *)NULL) {
					while (*uri == ' '){			/* Skip the white spaces before the name		*/
						uri++;
					}
					URIend = strchr(uri, ' ');		/* Set URL to point to the file name			*/
					if (URIend != NULL) {			/* Terminate the string for the file name		*/
						URIchar = *URIend;
						*URIend = '\0';				/* Done by finding the last white space			*/
					}
					if (0 == strcmp(uri, "/")) {	/* Root remaps to the default page				*/
						uri = "/default.html";
						if (0 != F_OPEN(Fdsc, uri)) {	/* Check if default.html exists				*/
							F_CLOSE(Fdsc);			/* It exist, will use it						*/
						}
						else {						/* It does not exists, try with default.shtml	*/
							uri = "/default.shtml";
						}
					}
					VerbRqst = g_MyVerbs[ii].Verb;	/* This is the method to handle					*/
				}
				break;
			}
		}

		FisOpen  = 0;								/* The file to send back is not yet open		*/
		CopyFile = 0;								/* We will decide to either copy the file as is	*/
		PageLen  = 0;								/* or to output a processed buffer (SSI)		*/
		switch(VerbRqst) {							/* -------------------------------------------- */
		case V_GET:									/* GET											*/
			FisOpen = F_OPEN(Fdsc, uri);			/* Try opening the file							*/
			if (FisOpen == 0) {						/* The file did not open, it could be CGIs		*/
				uri = CGIprocess(uri);				/* Check and try processing CGIs				*/
				if (uri != NULL) {					/* Try opening result of CGI if was valid CGI	*/
					FisOpen = F_OPEN(Fdsc, uri);	/* If URI is NULL, FisOpen already holds 0		*/
				}
			}
			break;

		case V_POST:								/* Place holders for unsupported methods		*/
			if (URIchar != '\0') {					/* Make sure to not try to analyse garbage		*/
				uri = POSTprocess(uri, URIend+1);
				if (uri != NULL) {					/* Try opening result of POST if was valid POST	*/
					FisOpen = F_OPEN(Fdsc, uri);	/* If URI is NULL, FisOpen already holds 0		*/
				}
			}
			break;

		case V_HEAD:
			break;
		case V_PUT:
			break;
		case V_DELETE:
			break;
		case V_TRACE:
			break;
		case V_OPTIONS:
			break;
		case V_CONNECT:
			break;
		case V_PATCH:
			break;
		default:									/* V_UNKNOWN_									*/
			break;
		}

		if (FisOpen != 0) {							/* The file is open, we can process it now		*/
			F_PATCH_OPEN(Fdsc);						/* Used for in-memory to not modify fs.c		*/
			if ((NULL != strstr(uri, ".shtm"))		/* Check if the file contains SSIs				*/
			||  (NULL != strstr(uri, ".stm"))) {	/* SSI file extensions: .stm, .stml, .shtml		*/
				SSIvarUpdate();						/* Make sure the local variables are up to date	*/
													/* Copy the file contents in g_PageInBuf		*/
				F_READ(Fdsc, &g_PageInBuf[PageLen], sizeof(g_PageInBuf)-1, &Fsize);
				if ((int)Fsize == (sizeof(g_PageInBuf)-1)) {
					MTXLOCK_STDIO();
					puts("httpserver_socket.c: File too big for g_PageInBuf");
					MTXUNLOCK_STDIO();
				}
				g_PageInBuf[(int)Fsize] = '\0';		/* Terminate the input buffer as a string		*/
				PageLen = SSIexpand(&g_PageOutBuf[0], sizeof(g_PageOutBuf), &g_PageInBuf[0]);
			}
			else {									/* Not a SSI file, direct copy from the file	*/
				CopyFile = 1;						/* to the client								*/
			}
		}

		if ((CopyFile == 0)							/* When file not found, send the 404 error		*/
		&&  (PageLen  == 0)							/* Will be copied if opening successful			*/
		&&  (Len != 0)) {							/* If the request is empty, do nothing			*/
			ii = 0;
			do {
				CopyFile = (0 != F_OPEN(Fdsc, "/404.html"));
				if (CopyFile == 0) {				/* Something is wrong as the 404 file cannot be	*/
					if (ii == 0) {
						MTXLOCK_STDIO();
						puts("**** ERROR: Failed to access the file system");
						MTXUNLOCK_STDIO();
					}
					if (0 != F_MNT()) {
						MTXLOCK_STDIO();
						if (ii == 0) {
							puts("**** ERROR: Failed to mount the SD/MMC card");
							puts("            Make sure the SD/MMC is inserted");
						}
						puts("            Trying to mount it in 5 seconds");
						MTXUNLOCK_STDIO();
						TSKsleep(5*OS_TICK_PER_SEC);
					}
					ii = 1;
				}
			} while (CopyFile == 0);
			if (ii != 0) {
				MTXLOCK_STDIO();
				puts("            OK, File System mounted");
				MTXUNLOCK_STDIO();				
			}
		}

		if (CopyFile != 0) {						/* Request to copy the file contents as is		*/
			F_PATCH_OPEN(Fdsc);						/* Used for in-memory to not modify fs.c		*/
			FisOpen = 1;
			do {
				F_READ(Fdsc, &g_PageOutBuf[0], sizeof(g_PageOutBuf), &Fsize);
				write(conn, (const void *)&g_PageOutBuf[0], Fsize);
			} while (Fsize != 0);
		}
		else if (PageLen != 0) {					/* Request to send a processed buffer			*/
			write(conn, (const void *)&g_PageOutBuf[0], PageLen);
		}
		if (FisOpen != 0) {
			F_CLOSE(Fdsc);
		}
	}

	close(conn);									/* Close connection socket						*/

	return;
}

/* EOF */
