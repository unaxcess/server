/*
** ICE - Internet Communications Engine
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** ICE.h: Main wrapper for server
*/

#ifndef _ICE_H_
#define _ICE_H_

// EDF messages
#define MSG_RQ_INVALID "rq_invalid"

// Shared lib defines

#ifdef WIN32
#define ICELIBFN __declspec(dllexport)
#else
#define ICELIBFN
#endif

// Reserved status values
#define STATUS_INIT 0
#define STATUS_EDF 1

// Server startup options
#define ICELIB_RECOVER 1

// Server load / unload options
#define ICELIB_RELOAD 1

// Server background function returns
#define ICELIB_QUIT 1
#define ICELIB_WRITE 2
#define ICELIB_RESTART 4

// Connection background function returns
#define ICELIB_CLOSE 1

// Type definitions for server / connection / request functions
typedef bool (*LIBSERVERFN)(EDF *, int);
typedef bool (*LIBSERVERTMFN)(EDF *, long, int);
typedef int (*LIBSERVERBGFN)(EDF *);
typedef bool (*LIBCONNFN)(EDFConn *, EDF *);
typedef bool (*LIBCONNREQUESTFN)(EDFConn *, EDF *, char *, EDF *, int);
typedef LIBCONNFN LIBCONNBGFN;
typedef EDF *(*LIBINTERREADFN)(EDFConn *, EDF *, unsigned char);
typedef bool (*LIBINTERWRITEFN)(EDFConn *, EDF *, EDF *);
typedef bool (*LIBMSGFN)(EDFConn *, EDF *, EDF *, EDF *);

int ConnCount();
EDFConn *ConnList(int iConnNum);
double ConnSent();
double ConnReceived();

#endif
