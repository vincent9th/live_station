#include "xvrRTSPChannelFactory.h"
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include "KULog.h"
//#include "uv_mutex_helper.hpp"

using namespace boost;

XvrRtspChannelDevice::XvrRtspChannelDevice(std::string streamid)
  : streamid_(streamid) {
  
}

XvrRtspChannelDevice::~XvrRtspChannelDevice() {
  LOGI((char*)"~XvrRtspChannelDevice");
}

std::string XvrRtspChannelDevice::stream_id()
{
  return streamid_;
}

///////////////////////////////// XvrRtspChannelDevice //////////////////////////////////////////////////////////////////////
XvrRtspChannelFactory::XvrRtspChannelFactory() {
  LOGI((char*)"new instance");
}

XvrRtspChannelFactory *XvrRtspChannelFactory::getInstance(){
  if(!instance_)
  {
    instance_ = new XvrRtspChannelFactory();
  }
  return instance_;
}

boost::shared_ptr<XvrRtspChannelDevice> XvrRtspChannelFactory::newRtspChannelDevice(std::string streamid) {
  shared_ptr<XvrRtspChannelDevice> pDevice;
  weak_ptr<XvrRtspChannelDevice>& wkDevice = channel_[streamid];
  pDevice = wkDevice.lock();
  LOGI((char*)"channel:%s %s", streamid.c_str(), !pDevice ? "not exist" : "exist");
  if(!pDevice)
  {
    pDevice.reset(new XvrRtspChannelDevice(streamid),
        bind(&XvrRtspChannelFactory::deleteRtspChannel, this, _1));
    wkDevice = pDevice;
    return pDevice;
  }
  return nullptr;
}

void XvrRtspChannelFactory::deleteRtspChannel(XvrRtspChannelDevice *channel)
{
  if(channel)
  {
    LOGI((char*)"delete channel:%s", channel->stream_id().c_str());
    channel_.erase(channel->stream_id());
  }
  delete channel;
}

XvrRtspChannelFactory* XvrRtspChannelFactory::instance_ = nullptr;

