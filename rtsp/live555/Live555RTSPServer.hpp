#ifndef _LIVE555_RTSP_SERVER_HPP_
#define _LIVE555_RTSP_SERVER_HPP_


#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <string.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"


class Live555RTSPServer {
public:
	Live555RTSPServer();
	~Live555RTSPServer();
	void Stop(void);
	int ThreadJoin();
	int SetRTSPServerPort(unsigned short Port);
	char *rtspURLPrefix(int clientSocket = 1);
	int createServerMediaSession(ServerMediaSession* &sms, char const* streamName = NULL, char const* info = NULL,
	    char const* description = NULL, Boolean isSSM = False, char const* miscSDPLines = NULL,
	    unsigned maxSize = 2000000);
	int addSubsession(ServerMediaSession* sms, ServerMediaSubsession* subsession);
	int Start();
	int addServerMediaSession(ServerMediaSession* serverMediaSession);
	UsageEnvironment* GetUsageEnvironmentObj();

	static void *SingleStepThread(void* arg);

protected:
	BasicTaskScheduler0*        m_pBasicTaskScheduler0;
	UsageEnvironment*           m_pUsageEnvironment;
	UserAuthenticationDatabase* m_pUserAuthenticationDatabase;
	RTSPServer*                 m_pRTSPServer;
	void* m_Thread_handle;
	volatile int m_iStartFlag;
	volatile int m_iPauseFlag;
	pthread_mutex_t m_ThreadMutex;
	char  m_UrlBuffer[1024];
};

#endif