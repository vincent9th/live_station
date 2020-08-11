cmake_minimum_required (VERSION 2.6)

set (MP3_SOURCE_SRCS MP3FileSource.cpp MP3Transcoder.cpp MP3ADU.cpp
 MP3ADUdescriptor.cpp MP3ADUinterleaving.cpp MP3ADUTranscoder.cpp MP3StreamState.cpp
 MP3Internals.cpp MP3InternalsHuffman.cpp MP3InternalsHuffmanTable.cpp MP3ADURTPSource.cpp)

set (MPEG_SOURCE_SRCS MPEG1or2Demux.cpp MPEG1or2DemuxedElementaryStream.cpp
 MPEGVideoStreamFramer.cpp MPEG1or2VideoStreamFramer.cpp MPEG1or2VideoStreamDiscreteFramer.cpp
 MPEG4VideoStreamFramer.cpp MPEG4VideoStreamDiscreteFramer.cpp
 H264or5VideoStreamFramer.cpp H264or5VideoStreamDiscreteFramer.cpp H264VideoStreamFramer.cpp
 H264VideoStreamDiscreteFramer.cpp H265VideoStreamFramer.cpp H265VideoStreamDiscreteFramer.cpp
 MPEGVideoStreamParser.cpp MPEG1or2AudioStreamFramer.cpp MPEG1or2AudioRTPSource.cpp MPEG4LATMAudioRTPSource.cpp
 MPEG4ESVideoRTPSource.cpp MPEG4GenericRTPSource.cpp
 ${MP3_SOURCE_SRCS} MPEG1or2VideoRTPSource.cpp MPEG2TransportStreamMultiplexor.cpp MPEG2TransportStreamFromPESSource.cpp
 MPEG2TransportStreamFromESSource.cpp MPEG2TransportStreamFramer.cpp MPEG2TransportStreamAccumulator.cpp ADTSAudioFileSource.cpp)

#JPEG_SOURCE_SRCS JPEGVideoSource.cpp JPEGVideoRTPSource.cpp JPEG2000VideoStreamFramer.cpp JPEG2000VideoStreamParser.cpp JPEG2000VideoRTPSource.cpp
set (JPEG_SOURCE_SRCS JPEGVideoSource.cpp JPEGVideoRTPSource.cpp JPEG2000VideoRTPSource.cpp)
set (H263_SOURCE_SRCS H263plusVideoRTPSource.cpp H263plusVideoStreamFramer.cpp H263plusVideoStreamParser.cpp)
set (AC3_SOURCE_SRCS AC3AudioStreamFramer.cpp AC3AudioRTPSource.cpp)
set (DV_SOURCE_SRCS DVVideoStreamFramer.cpp DVVideoRTPSource.cpp)
set (MP3_SINK_SRCS MP3ADURTPSink.cpp)
set (MPEG_SINK_SRCS MPEG1or2AudioRTPSink.cpp ${MP3_SINK_SRCS} MPEG1or2VideoRTPSink.cpp MPEG4LATMAudioRTPSink.cpp MPEG4GenericRTPSink.cpp MPEG4ESVideoRTPSink.cpp)
set (JPEG_SINK_SRCS JPEGVideoRTPSink.cpp JPEG2000VideoRTPSink.cpp)
set (H263_SINK_SRCS H263plusVideoRTPSink.cpp)
set (H264_OR_5_SINK_SRCS H264or5VideoRTPSink.cpp H264VideoRTPSink.cpp H265VideoRTPSink.cpp)
set (DV_SINK_SRCS DVVideoRTPSink.cpp)
set (AC3_SINK_SRCS AC3AudioRTPSink.cpp)

set (MISC_SOURCE_SRCS MediaSource.cpp FramedSource.cpp FramedFileSource.cpp FramedFilter.cpp ByteStreamFileSource.cpp ByteStreamMultiFileSource.cpp
 ByteStreamMemoryBufferSource.cpp BasicUDPSource.cpp DeviceSource.cpp AudioInputDevice.cpp WAVAudioFileSource.cpp
 ${MPEG_SOURCE_SRCS} ${JPEG_SOURCE_SRCS} ${H263_SOURCE_SRCS} ${AC3_SOURCE_SRCS} ${DV_SOURCE_SRCS} AMRAudioSource.cpp AMRAudioFileSource.cpp InputFile.cpp StreamReplicator.cpp)
set (MISC_SINK_SRCS MediaSink.cpp FileSink.cpp BasicUDPSink.cpp AMRAudioFileSink.cpp H264or5VideoFileSink.cpp H264VideoFileSink.cpp H265VideoFileSink.cpp OggFileSink.cpp
 ${MPEG_SINK_SRCS} ${JPEG_SINK_SRCS} ${H263_SINK_SRCS} ${H264_OR_5_SINK_SRCS} ${DV_SINK_SRCS} ${AC3_SINK_SRCS}
 VorbisAudioRTPSink.cpp TheoraVideoRTPSink.cpp VP8VideoRTPSink.cpp VP9VideoRTPSink.cpp GSMAudioRTPSink.cpp
 SimpleRTPSink.cpp AMRAudioRTPSink.cpp T140TextRTPSink.cpp TCPStreamSink.cpp OutputFile.cpp RawVideoRTPSink.cpp)
set (MISC_FILTER_SRCS uLawAudioFilter.cpp)
set (TRANSPORT_STREAM_TRICK_PLAY_SRCS MPEG2IndexFromTransportStream.cpp MPEG2TransportStreamIndexFile.cpp MPEG2TransportStreamTrickModeFilter.cpp)

set (RTP_SOURCE_SRCS RTPSource.cpp MultiFramedRTPSource.cpp SimpleRTPSource.cpp H261VideoRTPSource.cpp
 H264VideoRTPSource.cpp H265VideoRTPSource.cpp QCELPAudioRTPSource.cpp AMRAudioRTPSource.cpp
 VorbisAudioRTPSource.cpp TheoraVideoRTPSource.cpp VP8VideoRTPSource.cpp VP9VideoRTPSource.cpp RawVideoRTPSource.cpp)
set (RTP_SINK_SRCS RTPSink.cpp MultiFramedRTPSink.cpp AudioRTPSink.cpp VideoRTPSink.cpp TextRTPSink.cpp)
set (RTP_INTERFACE_SRCS RTPInterface.cpp)
set (RTP_SRCS ${RTP_SOURCE_SRCS} ${RTP_SINK_SRCS} ${RTP_INTERFACE_SRCS})

set (RTCP_SRCS RTCP.cpp rtcp_from_spec.c)
set (GENERIC_MEDIA_SERVER_SRCS GenericMediaServer.cpp)
set (RTSP_SRCS RTSPServer.cpp RTSPServerRegister.cpp RTSPClient.cpp RTSPCommon.cpp RTSPServerSupportingHTTPStreaming.cpp RTSPRegisterSender.cpp)
set (SIP_SRCS SIPClient.cpp)

set (SESSION_SRCS MediaSession.cpp ServerMediaSession.cpp PassiveServerMediaSubsession.cpp OnDemandServerMediaSubsession.cpp FileServerMediaSubsession.cpp MPEG4VideoFileServerMediaSubsession.cpp H264VideoFileServerMediaSubsession.cpp H265VideoFileServerMediaSubsession.cpp H263plusVideoFileServerMediaSubsession.cpp WAVAudioFileServerMediaSubsession.cpp AMRAudioFileServerMediaSubsession.cpp MP3AudioFileServerMediaSubsession.cpp MPEG1or2VideoFileServerMediaSubsession.cpp MPEG1or2FileServerDemux.cpp MPEG1or2DemuxedServerMediaSubsession.cpp MPEG2TransportFileServerMediaSubsession.cpp ADTSAudioFileServerMediaSubsession.cpp DVVideoFileServerMediaSubsession.cpp AC3AudioFileServerMediaSubsession.cpp MPEG2TransportUDPServerMediaSubsession.cpp ProxyServerMediaSession.cpp)

set (QUICKTIME_SRCS QuickTimeFileSink.cpp QuickTimeGenericRTPSource.cpp)
set (AVI_SRCS AVIFileSink.cpp)

set (MATROSKA_FILE_SRCS MatroskaFile.cpp MatroskaFileParser.cpp EBMLNumber.cpp MatroskaDemuxedTrack.cpp)
set (MATROSKA_SERVER_MEDIA_SUBSESSION_SRCS MatroskaFileServerMediaSubsession.cpp MP3AudioMatroskaFileServerMediaSubsession.cpp)
set (MATROSKA_RTSP_SERVER_SRCS MatroskaFileServerDemux.cpp ${MATROSKA_SERVER_MEDIA_SUBSESSION_SRCS})
set (MATROSKA_SRCS ${MATROSKA_FILE_SRCS} ${MATROSKA_RTSP_SERVER_SRCS})

set (OGG_FILE_SRCS OggFile.cpp OggFileParser.cpp OggDemuxedTrack.cpp)
set (OGG_SERVER_MEDIA_SUBSESSION_SRCS OggFileServerMediaSubsession.cpp)
set (OGG_RTSP_SERVER_SRCS OggFileServerDemux.cpp ${OGG_SERVER_MEDIA_SUBSESSION_SRCS})
set (OGG_SRCS ${OGG_FILE_SRCS} ${OGG_RTSP_SERVER_SRCS})

set (TRANSPORT_STREAM_DEMUX_SRCS MPEG2TransportStreamDemux.cpp MPEG2TransportStreamDemuxedTrack.cpp
 MPEG2TransportStreamParser.cpp MPEG2TransportStreamParser_PAT.cpp MPEG2TransportStreamParser_PMT.cpp MPEG2TransportStreamParser_STREAM.cpp)

set (MISC_SRCS BitVector.cpp StreamParser.cpp DigestAuthentication.cpp ourMD5.cpp Base64.cpp Locale.cpp)

set (LIVEMEDIA_LIB_SRCS Media.cpp ${MISC_SOURCE_SRCS} ${MISC_SINK_SRCS} ${MISC_FILTER_SRCS} ${RTP_SRCS} ${RTCP_SRCS} ${GENERIC_MEDIA_SERVER_SRCS}
 ${RTSP_SRCS} ${SIP_SRCS} ${SESSION_SRCS} ${QUICKTIME_SRCS} ${AVI_SRCS} ${TRANSPORT_STREAM_TRICK_PLAY_SRCS} ${MATROSKA_SRCS} ${OGG_SRCS} ${TRANSPORT_STREAM_DEMUX_SRCS} ${MISC_SRCS})

include_directories(${LIVE_GROUPSOCK_INCLUDE})
include_directories(${LIVE_COMM_INCLUDE})
include_directories(${LIVE_USAGE_ENV_INCLUDE})
include_directories(${LIVE_LIVEMEDIA_INCLUDE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNEWLOCALE_NOT_USED")

add_library(liveMedia_a STATIC ${LIVEMEDIA_LIB_SRCS})
