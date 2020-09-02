#ifndef XVR_RTSP_CLIENT_FACTORY_H
#define XVR_RTSP_CLIENT_FACTORY_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
//#include "uv_mutex_helper.hpp"

class XvrRtspChannelDevice
{
public:
  XvrRtspChannelDevice(std::string streamid);
  virtual ~XvrRtspChannelDevice();
  std::string stream_id();
  
private:
  std::string streamid_;
};

class XvrRtspChannelFactory
{
public:
  XvrRtspChannelFactory();
  
  static XvrRtspChannelFactory *getInstance();
  boost::shared_ptr<XvrRtspChannelDevice> newRtspChannelDevice(std::string streamid);
  
private:
  void deleteRtspChannel(XvrRtspChannelDevice *channel);
  std::map<std::string, boost::weak_ptr<XvrRtspChannelDevice> > channel_;

private:
  static XvrRtspChannelFactory* instance_;
};


#endif
