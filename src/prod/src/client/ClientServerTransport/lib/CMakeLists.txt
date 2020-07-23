set( LINUX_SOURCES
  ../ClientProtocolVersionHeader.cpp
  ../ClientServerPassThroughTransport.cpp
  ../ClientServerTcpTransport.cpp
  ../ClusterManagerTcpMessage.cpp
  ../ContainerOperationTcpMessage.cpp
  ../CopyRequest.cpp
  ../ImageStoreBaseRequest.cpp
  ../FaultAnalysisServiceTcpMessage.cpp
  ../FileStoreServiceTcpMessage.cpp
  ../FileTransferTcpMessage.cpp
  ../HealthManagerTcpMessage.cpp
  ../InfrastructureServiceTcpMessage.cpp
  ../InternalListReply.cpp
  ../ListReply.cpp
  ../ListRequest.cpp
  ../NamingTcpMessage.cpp
  ../NotificationClientConnectionRequestBody.cpp
  ../NotificationClientSynchronizationRequestBody.cpp
  ../PassThroughReceiverContext.cpp
  ../PassThroughSendTarget.cpp
  ../QueryTcpMessage.cpp
  ../RepairManagerTcpMessage.cpp
  ../ReportFaultReplyMessageBody.cpp
  ../ReportFaultRequestMessageBody.cpp
  ../ServiceLocationNotificationReplyBody.cpp
  ../ServiceNotificationRequestBody.cpp
  ../ShareLocationReply.cpp
  ../StartNodeRequestMessageBody.cpp
  ../TokenValidationServiceTcpMessage.cpp
  ../UpgradeOrchestrationServiceTcpMessage.cpp
  ../CentralSecretServiceMessage.cpp
  ../ResourceManagerMessage.cpp
  ../UploadRequest.cpp
  ../createuploadsessionrequest.cpp
  ../deleteuploadsessionrequest.cpp
  ../uploadchunkrequest.cpp
  ../UploadChunkContentRequest.cpp
  ../uploadsessionrequest.cpp
  ../VolumeOperationTcpMessage.cpp
  ../FileUploadCreateRequestHeader.cpp
  ../GatewayResourceManagerTcpMessage.cpp
)

include_directories("..")

add_library(objects_ClientServerTransport OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ClientServerTransport ../stdafx.h)
add_library(${lib_ClientServerTransport} STATIC $<TARGET_OBJECTS:objects_ClientServerTransport>)

set_target_properties(${lib_ClientServerTransport} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

