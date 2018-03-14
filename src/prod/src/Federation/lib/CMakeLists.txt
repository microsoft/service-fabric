set( LINUX_SOURCES
    ../ArbitrationOperation.cpp
    ../ArbitrationReplyBody.cpp
    ../ArbitrationRequestBody.cpp
    ../ArbitrationTable.cpp
    ../ArbitrationType.cpp
    ../BroadcastManager.cpp
    ../BroadcastReplyContext.cpp
    ../BroadcastAckReceiverContext.cpp
    ../BroadcastRequestReceiverContext.cpp
    ../BroadcastForwardContext.cpp
    ../Constants.cpp
    ../FederationConfig.cpp
    ../FederationEventSource.cpp
    ../FederationMessage.cpp
    ../FederationSubsystem.cpp
    ../GlobalStore.cpp
    ../GlobalTimeManager.cpp
    ../IMessageFilter.cpp
    ../IRouter.cpp
    ../JoinLockManager.cpp
    ../JoinManager.cpp
    ../Multicast.cpp
    ../MulticastAckReceiverContext.cpp
    ../MulticastForwardContext.cpp
    ../MulticastManager.cpp
    ../NodeIdRange.cpp
    ../NodeIdRangeTable.cpp
    ../NodePhase.cpp
    ../NodeRing.cpp
    ../OneWayReceiverContext.cpp
    ../PartnerNode.cpp
    ../PingManager.cpp
    ../PointToPointManager.cpp
    ../PToPActor.cpp
    ../ReceiverContext.cpp
    ../RequestReceiverContext.cpp
    ../RouteAsyncOperation.cpp
    ../RoutedOneWayReceiverContext.cpp
    ../RoutedRequestReceiverContext.cpp
    ../RoutingManager.cpp
    ../RoutingTable.cpp
    ../RoutingToken.cpp
    ../SeedNodeProxy.cpp
    ../SendMessageAction.cpp
    ../SerializableActivationFactory.cpp
    ../SerializableWithActivation.cpp    
    ../SharableProxy.cpp
    ../SiteNode.cpp
    ../StateMachineAction.cpp
    ../TicketGap.cpp
    ../ThrottleManager.cpp
    ../TimeRange.cpp
    ../UpdateManager.cpp
    ../VoteEntry.cpp
    ../VoteManager.cpp
    ../VoteProxy.cpp
    ../VoterStore.cpp
    ../WindowsAzureProxy.cpp
)

include_directories("..")
include_directories("../pch")

add_library(objects_Federation OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Federation ../stdafx.h)
add_library(${lib_Federation} STATIC $<TARGET_OBJECTS:objects_Federation>)

set_target_properties(${lib_Federation} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

