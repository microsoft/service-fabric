set( LINUX_SOURCES
  ../AcceptThrottle.cpp
  ../ActionHeader.cpp
  ../Actor.cpp
  ../ActorHeader.cpp
  ../BiqueChunkIterator.cpp
  ../BiqueRangeStream.cpp
  ../BiqueWriteStream.cpp
  ../ByteBique.cpp
  ../ClaimsMessage.cpp
  ../ClientAuthHeader.cpp
  ../ClientRoleHeader.cpp
  ../Constants.cpp
  ../CredentialType.cpp
  ../DatagramTransportFactory.cpp
  ../Demuxer.cpp
  ../DuplexRequestReply.cpp
  ../FabricActivityHeader.cpp
  ../IConnection.cpp
  ../IDatagramTransport.cpp
  ../IdempotentHeader.cpp
  ../IListenSocket.cpp
  ../IpcClient.cpp
  ../IpcDemuxer.cpp
  ../IpcHeader.cpp
  ../IpcReceiverContext.cpp
  ../IpcServer.cpp
  ../ISendTarget.cpp
  ../ListenInstance.cpp
  ../ListenSocket.Linux.cpp
  ../LTBufferFactory.cpp
  ../LTReceiveBuffer.cpp
  ../LTSendBuffer.cpp
  ../MemoryTransport.cpp
  ../Message.cpp
  ../MessageHeadersCollection.cpp
  ../MessageHeaderId.cpp
  ../MessageHeaders.cpp
  ../MessageHeaderTrace.cpp
  ../MessageId.cpp
  ../MessageIdHeader.cpp
  ../MulticastDatagramSender.cpp
  ../MulticastSendTarget.cpp
  ../PerfCounters.cpp
  ../ReceiveBuffer.cpp
  ../ReceiverContext.cpp
  ../RequestAsyncOperation.cpp
  ../RequestInstanceHeader.cpp
  ../requestreply.cpp
  ../RequestTable.cpp
#  ../SecurityBuffers.cpp
  ../SecurityContext.cpp
  ../SecurityContextSsl.cpp
#  ../SecurityContextWin.cpp
  ../SecurityCredentials.cpp
  ../SecurityNegotiationHeader.cpp
  ../SecurityUtil.cpp
  ../SendBuffer.cpp
  ../ServerAuthHeader.cpp
  ../stdafx.cpp
  ../TcpBufferFactory.cpp
  ../TcpConnection.cpp
  ../TcpConnectionState.cpp
  ../TcpDatagramTransport.cpp
  ../TcpFrameHeader.cpp
  ../TcpReceiveBuffer.cpp
  ../TcpSendBuffer.cpp
  ../TcpSendTarget.cpp
  ../TcpTransportUtility.cpp
  ../Throttle.cpp
  ../TimeoutHeader.cpp
  ../Trace.cpp
  ../TransportConfig.cpp
  ../TransportFlags.cpp
  ../TransportPriority.cpp
  ../TransportSecurity.cpp
  ../TransportTable.cpp
  ../UnreliableTransportConfig.cpp
  ../UnreliableTransportConfiguration.cpp
  ../UnreliableTransport.cpp
  ../UnreliableTransportSpecification.cpp
)

set(headers
  ../MessageHeader.h
)

include_directories("..")

add_library(objects_Transport OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Transport ../stdafx.h)
add_library(${lib_Transport} STATIC $<TARGET_OBJECTS:objects_Transport>)

set_target_properties(${lib_Transport} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

