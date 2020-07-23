set( LINUX_SOURCES
    ../CacheModeHeader.cpp
    ../Constants.cpp
    ../NamingMessage.cpp
    ../NamingCommonEventSource.cpp
    ../NamingConfig.cpp
    ../NamingService.cpp
    ../NamingServiceCuidCollection.cpp
    ../NamingUtility.cpp
    ../BroadcastEventManager.cpp
    ../BroadcastRequestEntry.cpp
    ../BroadcastRequestVector.cpp
    ../BroadcastWaiters.cpp
    ../ClientAccessConfig.cpp
    ../ClientRegistrationTable.cpp
    ../EntreeService.cpp
    ../EntreeService.AdminRequestAsyncOperationBase.cpp
    ../EntreeService.CreateNameAsyncOperation.cpp
    ../EntreeService.CreateServiceAsyncOperation.cpp
    ../EntreeService.CreateSystemServiceAsyncOperation.cpp
    ../EntreeService.DeleteNameAsyncOperation.cpp
    ../EntreeService.DeleteServiceAsyncOperation.cpp
    ../EntreeService.DeleteSystemServiceAsyncOperation.cpp
    ../EntreeService.EnumeratePropertiesAsyncOperation.cpp
    ../EntreeService.EnumerateSubnamesAsyncOperation.cpp
    ../EntreeService.ForwardToFileStoreServiceAsyncOperation.cpp
    ../EntreeService.ForwardToServiceOperation.cpp
    ../EntreeService.GetServiceDescriptionAsyncOperation.cpp
    ../EntreeService.NameExistsAsyncOperation.cpp
    ../EntreeService.NamingRequestAsyncOperationBase.cpp
    ../EntreeService.PingAsyncOperation.cpp
    ../EntreeService.PropertyBatchAsyncOperation.cpp
    ../EntreeService.DeactivateNodeAsyncOperation.cpp
    ../EntreeService.ActivateNodeAsyncOperation.cpp
    ../EntreeService.DeactivateNodesBatchAsyncOperation.cpp
    ../EntreeService.RemoveNodeDeactivationsAsyncOperation.cpp
    ../EntreeService.GetNodeDeactivationStatusAsyncOperation.cpp
    ../EntreeService.NodeStateRemovedAsyncOperation.cpp
    ../EntreeService.RecoverPartitionsAsyncOperation.cpp
    ../EntreeService.RecoverPartitionAsyncOperation.cpp
    ../EntreeService.RecoverServicePartitionsAsyncOperation.cpp
    ../EntreeService.RecoverSystemPartitionsAsyncOperation.cpp        
    ../EntreeService.ResetPartitionLoadAsyncOperation.cpp
    ../EntreeService.ReportFaultAsyncOperation.cpp
    ../EntreeService.FMQueryAsyncOperation.cpp
    ../EntreeService.ResolveNameOwnerAsyncOperation.cpp
    ../EntreeService.ResolvePartitionAsyncOperation.cpp
    ../EntreeService.ResolveServiceAsyncOperation.cpp
    ../EntreeService.ResolveSystemServiceGatewayAsyncOperation.cpp
    ../EntreeService.ResolveSystemServiceAsyncOperation.cpp
    ../EntreeService.PrefixResolveServiceAsyncOperation.cpp
    ../EntreeService.RequestAsyncOperationBase.cpp
    ../EntreeService.ServiceLocationNotificationAsyncOperation.cpp
    ../EntreeService.ServiceResolutionAsyncOperationBase.cpp
    ../EntreeService.SendToNodeOperation.cpp
    ../EntreeService.UpdateServiceAsyncOperation.cpp
    ../EntreeService.QueryRequestOperation.cpp
    ../EntreeService.GetQueryListAsyncOperation.cpp
    ../EntreeService.ProcessGatewayMessageAsyncOperation.cpp
    ../EntreeService.ProcessServiceNotificationRequestAsyncOperation.cpp
    ../EntreeService.ForwardToTokenValidationServiceAsyncOperation.cpp
    ../EntreeService.StartNodeAsyncOperation.cpp
    ../EntreeService.Test_TestNamespaceManagerAsyncOperation.cpp
    ../EntreeService.ProcessQueryAsyncOperation.cpp
    ../EntreeService.GetApplicationNameAsyncOperation.cpp
    ../EntreeService.CreateNetworkAsyncOperation.cpp
    ../EntreeService.DeleteNetworkAsyncOperation.cpp
    ../EntreeServiceProxy.cpp
    ../EntreeServiceProxy.RequestAsyncOperationBase.cpp
    ../EntreeServiceIpcChannelBase.cpp
    ../EntreeServiceToProxyIpcChannel.cpp
    ../ProxyToEntreeServiceIpcChannel.cpp
    ../EntreeServiceTransport.cpp
    ../FabricGatewayManager.cpp
    ../FileTransferGateway.cpp
    ../GatewayCounters.cpp
    ../GatewayManagerWrapper.cpp
    ../GatewayProperties.cpp
    ../GatewayNotificationReplyBuilder.cpp
    ../GatewayRetryHeader.cpp
    ../ResolveNameLocationAsyncOperation.cpp    
    ../ServiceNotificationManagerProxy.cpp
    ../TestTokenAuthenticator.cpp
    ../TestGatewayManager.cpp
    ../EntreeService.ToggleVerboseServicePlacementHealthReportingAsyncOperation.cpp   
)

include_directories("..")
 

add_library(objects_EntreeService OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_EntreeService ../stdafx.h)
add_library(${lib_EntreeService} STATIC $<TARGET_OBJECTS:objects_EntreeService>)

set_target_properties(${lib_EntreeService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 


