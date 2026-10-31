#include "Live555RTSPServer.hpp"

Live555RTSPServer::Live555RTSPServer()
	: m_pBasicTaskScheduler0(NULL)
	, m_pUsageEnvironment(NULL)
	, m_pUserAuthenticationDatabase(NULL)
	, m_pRTSPServer(NULL)
	, m_Thread_handle(NULL)
	, m_iStartFlag(0)
	, m_iPauseFlag(0)
{
	pthread_mutex_init(&m_ThreadMutex, NULL);
	m_pBasicTaskScheduler0 = BasicTaskScheduler::createNew();
	m_pUsageEnvironment = BasicUsageEnvironment::createNew(*m_pBasicTaskScheduler0);
	memset(m_UrlBuffer, 0, sizeof(m_UrlBuffer));
}

Live555RTSPServer::~Live555RTSPServer()
{
	m_iStartFlag = 0;
	ThreadJoin();

	if ( m_pRTSPServer ) {
		Medium::close(m_pRTSPServer);
		m_pRTSPServer = NULL;
	}
	if ( m_pUserAuthenticationDatabase ) {
		delete m_pUserAuthenticationDatabase;
		m_pUserAuthenticationDatabase = NULL;
	}
	if ( m_pUsageEnvironment ) {
		m_pUsageEnvironment->reclaim();
		m_pUsageEnvironment = NULL;
	}
	if ( m_pBasicTaskScheduler0 ) {
		BasicTaskScheduler* p_BasicTaskScheduler = (BasicTaskScheduler*)m_pBasicTaskScheduler0;
		delete p_BasicTaskScheduler;
		m_pBasicTaskScheduler0 = NULL;
	}
	pthread_mutex_destroy(&m_ThreadMutex);
}

void Live555RTSPServer::Stop(void)
{
	m_iStartFlag = 0;
	m_iPauseFlag = 1;
}

int Live555RTSPServer::ThreadJoin()
{
	pthread_mutex_lock(&m_ThreadMutex);
	m_iPauseFlag = 1;
	m_iStartFlag = 0;
	if ( m_Thread_handle==NULL ) {
		pthread_mutex_unlock(&m_ThreadMutex);
		return -1;
	}
	pthread_t *pthread_handle = (pthread_t *)m_Thread_handle;
	int iRet = pthread_join(*pthread_handle, NULL);
	free(m_Thread_handle);
	m_Thread_handle=NULL;
	pthread_mutex_unlock(&m_ThreadMutex);
	return iRet;
}

int Live555RTSPServer::SetRTSPServerPort(unsigned short Port)
{
	if ( m_pRTSPServer ) {
		return -1;
	}
	m_pRTSPServer=RTSPServer::createNew(*m_pUsageEnvironment, Port, m_pUserAuthenticationDatabase);
	if ( m_pRTSPServer==NULL ) {
		return -2;
	}
	return 0;
}

char *Live555RTSPServer::rtspURLPrefix(int clientSocket)
{
	if ( m_pRTSPServer ) {
		char *urlBuffer = m_pRTSPServer->rtspURLPrefix(clientSocket);
		if ( urlBuffer==NULL ) {
			return NULL;
		}
		memset(m_UrlBuffer, 0, sizeof(m_UrlBuffer));
		strncpy(m_UrlBuffer, urlBuffer, sizeof(m_UrlBuffer));
		delete[] urlBuffer;
		urlBuffer = NULL;
		return m_UrlBuffer;
	} else {
		return NULL;
	}
}

int Live555RTSPServer::createServerMediaSession(ServerMediaSession* &sms, char const* streamName, char const* info,
	    char const* description, Boolean isSSM, char const* miscSDPLines, unsigned maxSize)
{
	if ( m_pUsageEnvironment==NULL ) {
		return -1;
	}
	sms = ServerMediaSession::createNew(*m_pUsageEnvironment, streamName, info, description, isSSM, miscSDPLines);
	OutPacketBuffer::maxSize = maxSize;
	if ( sms==NULL ) {
		return -2;
	}
	return 0;
}

int Live555RTSPServer::addServerMediaSession(ServerMediaSession* serverMediaSession)
{
	if ( serverMediaSession==NULL ) {
		return -1;
	} else if ( m_pRTSPServer==NULL ) {
		return -2;
	}
	m_pRTSPServer->addServerMediaSession(serverMediaSession);
	return 0;
}

int Live555RTSPServer::addSubsession(ServerMediaSession* sms, ServerMediaSubsession* subsession)
{
	if ( sms==NULL ) {
		return -1;
	} else if ( subsession==NULL ) {
		return -2;
	}
	sms->addSubsession(subsession);
	return 0;
}

void *Live555RTSPServer::SingleStepThread(void* arg)
{
	Live555RTSPServer* p_This = (Live555RTSPServer*)arg;
#if defined(linux)
	prctl(PR_SET_NAME, "rtsp_server");
#endif
	while( p_This->m_iStartFlag ) {
		if ( p_This->m_iPauseFlag ) {
			sleep(1);
			continue;
		}
		p_This->m_pBasicTaskScheduler0->SingleStep();
	}
	return NULL;
}

UsageEnvironment* Live555RTSPServer::GetUsageEnvironmentObj()
{
	return m_pUsageEnvironment;
}

#include "H265LiveVideoServerMediaSubssion.hh"
#include "H264LiveVideoServerMediaSubssion.hh"

int Live555RTSPServer::addStream(const StreamConfig &cfg)
{
	if (m_pUsageEnvironment == NULL || m_pRTSPServer == NULL || cfg.getFrame == NULL || cfg.name.empty()) {
		return -1;
	}

	ServerMediaSession *sms = NULL;
	if (createServerMediaSession(sms, cfg.name.c_str(), NULL, NULL) != 0 || sms == NULL) {
		return -2;
	}

	ServerMediaSubsession *subSession = NULL;
	if (cfg.isH265) {
		subSession = H265LiveVideoServerMediaSubssion::createNew(*m_pUsageEnvironment, cfg.reuseFirstSource, cfg.getFrame);
	} else {
		subSession = H264LiveVideoServerMediaSubssion::createNew(*m_pUsageEnvironment, cfg.reuseFirstSource, cfg.getFrame);
	}

	if (subSession == NULL || addSubsession(sms, subSession) != 0) {
		Medium::close(sms);
		return -3;
	}

	if (addServerMediaSession(sms) != 0) {
		Medium::close(sms);
		return -4;
	}

	return 0;
}

int Live555RTSPServer::addVideoStream(const char* name, GetFrame* gf, bool h265, bool reuseFirstSource)
{
	StreamConfig cfg;
	if (name != NULL) {
		cfg.name = name;
	}
	cfg.getFrame = gf;
	cfg.isH265 = h265;
	cfg.reuseFirstSource = reuseFirstSource;
	return addStream(cfg);
}

int Live555RTSPServer::Start()
{
	pthread_mutex_lock(&m_ThreadMutex);
	m_iPauseFlag = 0;
	if ( m_Thread_handle ) {
		pthread_mutex_unlock(&m_ThreadMutex);
		return -1;
	} else if ( m_iStartFlag ) {
		pthread_mutex_unlock(&m_ThreadMutex);
		return -2;
	}
	m_Thread_handle = calloc(1, sizeof(pthread_t));
	m_iStartFlag = 1;
	int iRet = pthread_create((pthread_t *)m_Thread_handle, NULL, SingleStepThread, this);
	pthread_mutex_unlock(&m_ThreadMutex);
	return iRet;
}