set( LINUX_SOURCES
  DnsCache.cpp
  DnsExchangeOp.cpp
  DnsHealthMonitor.cpp
  DnsNodeCacheMonitor.cpp
  DnsRemoteQueryOp.cpp
  DnsResolveFqdnOp.cpp
  DnsResolveOp.cpp
  DnsService.cpp
  NetworkParams.cpp
  Utility.cpp
  DnsParser/BinaryReader.cpp
  DnsParser/BinaryWriter.cpp
  DnsParser/DnsAnswerRecordA.cpp
  DnsParser/DnsAnswerRecordBase.cpp
  DnsParser/DnsAnswerRecordOpt.cpp
  DnsParser/DnsAnswerRecordSrv.cpp
  DnsParser/DnsAnswerRecordTxt.cpp
  DnsParser/DnsFlags.cpp
  DnsParser/DnsMessage.cpp
  DnsParser/DnsMessageHeader.cpp
  DnsParser/DnsParser.cpp
  DnsParser/DnsQuestionRecord.cpp
  DnsParser/DnsText.cpp
  DnsParser/IDNConversionUtility.cpp
  Fabric/FabricResolve.cpp
  Fabric/FabricResolveOp.cpp
  Fabric/OperationCallback.cpp
  Fabric/FabricGetDnsNameOp.cpp
  StateMachine/StateMachineBase.cpp
  StateMachine/StateMachineQueue.cpp
  Transport/NetIoManager.cpp
  Transport/UdpWriteOp.cpp
  Transport/UdpReadOp.cpp
  Transport/UdpListener.cpp
  Transport/SocketAddress.cpp
  precomp/stdafx.cpp
)

include_directories(BEFORE "../linux_include")
include_directories(BEFORE "../include")
include_directories("../config")
include_directories("include")
include_directories("precomp")
include_directories("StateMachine")

add_library(objects_DnsService OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_DnsService precomp/stdafx.h)
add_library(${lib_DnsService} STATIC $<TARGET_OBJECTS:objects_DnsService>)

set_target_properties(${lib_DnsService} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

