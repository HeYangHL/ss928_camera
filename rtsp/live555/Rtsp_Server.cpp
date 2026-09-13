#include "Rtsp_Server.hpp"

void RtspServer::Init(void) {
  scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  RTSPServer *rtspServer = RTSPServer::createNew(*env, 8554, authDB);
  if (rtspServer == NULL) {
    *env << "Failed to create rtsp server : " << env->getResultMsg() << "\n";
    exit(1);
  }
  char const *descriptionString =
      "Session streamed by \"testOnDemandRTSPServer\"";
  char fWatchVariable = 0;

  char const *streamName = "sensor0_media";
  Boolean reuseFirstSource = true;
  ServerMediaSession *sms = ServerMediaSession::createNew(
      *env, streamName, streamName, descriptionString);
  int datalen = 0;
  unsigned char *databuf = NULL;
  databuf = (unsigned char *)malloc(1024);
  sms->addSubsession(H264LiveVideoServerMediaSubssion::createNew(*env, reuseFirstSource));
  rtspServer->addServerMediaSession(sms);
  announceStream(rtspServer, sms, streamName, "TCP Port 10000");

  char const *httpProtocolStr = "HTTP";
  if (rtspServer->setUpTunnelingOverHTTP(80) ||
      rtspServer->setUpTunnelingOverHTTP(8000) ||
      rtspServer->setUpTunnelingOverHTTP(8080)) {

    *env << "\n(We use port " << rtspServer->httpServerPortNum()
         << " for optional RTSP-over-" << httpProtocolStr << " tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-" << httpProtocolStr
         << " tunneling is not available.)\n";
  }

  env->taskScheduler().doEventLoop(&fWatchVariable); // does not return

  return;
}

void RtspServer::Stop(void)
{
  fWatchVariable = 1; 
  return;
}

void RtspServer::announceURL(RTSPServer *rtspServer, ServerMediaSession *sms) {
  if (rtspServer == NULL || sms == NULL)
    return; // sanity check

  UsageEnvironment &env = rtspServer->envir();

  env << "Play this stream using the URL ";
  if (weHaveAnIPv4Address(env)) {
    char *url = rtspServer->ipv4rtspURL(sms);
    env << "\"" << url << "\"";
    delete[] url;
    if (weHaveAnIPv6Address(env))
      env << " or ";
  }
  if (weHaveAnIPv6Address(env)) {
    char *url = rtspServer->ipv6rtspURL(sms);
    env << "\"" << url << "\"";
    delete[] url;
  }
  env << "\n";

  return;
}

void RtspServer::announceStream(RTSPServer *rtspServer, ServerMediaSession *sms,
                                char const *streamName,
                                char const *inputFileName) {
  UsageEnvironment &env = rtspServer->envir();

  env << "\n\"" << streamName << "\" stream, from the file \"" << inputFileName
      << "\"\n";
  announceURL(rtspServer, sms);
}