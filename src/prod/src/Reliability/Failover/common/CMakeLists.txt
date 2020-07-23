set( LINUX_SOURCES
    ActivateMessageBody.cpp
    ConfigurationMessageBody.cpp
    ConfigurationReplyMessageBody.cpp
    Constants.cpp
    DoReconfigurationMessageBody.cpp
    DeactivateMessageBody.cpp
    DeleteReplicaMessageBody.cpp
    FailoverConfig.cpp
    FailoverManagerId.cpp
    FailoverUnitDescription.cpp
    FailoverUnitFlags.cpp
    FailoverUnitInfo.cpp
    FailoverUnitMessageBody.cpp
    FailoverUnitReplyMessageBody.cpp
    GetLSNReplyMessageBody.cpp
    NodeActivateReplyMessageBody.cpp
    NodeActivateRequestMessageBody.cpp
    NodeDeactivateRequestMessageBody.cpp
    NodeDeactivateReplyMessageBody.cpp
    NodeDescription.cpp
    NodeFabricUpgradeReplyMessageBody.cpp
    NodeUpAckMessageBody.cpp
    NodeUpdateServiceReplyMessageBody.cpp
    NodeUpgradeMessageReplyBody.cpp
    NodeUpOperation.cpp
    NodeUpOperationFactory.cpp
    PersistenceState.cpp
    PlbMovementIgnoredReasons.cpp
    RALFUMUploadOperation.cpp
    RAReplicaMessageBody.cpp
    ReliabilityEventSource.cpp
    ReplicaDeactivationInfo.cpp
    ReplicaDescription.cpp
    ReplicaFlags.cpp
    ReplicaInfo.cpp
    ReplicaListMessageBody.cpp
    ReplicaMessageBody.cpp
    ReplicaReplyMessageBody.cpp
    ReplicaRole.cpp
    ReplicaStates.cpp
    ReplicaStatus.cpp
    ReplicaUpMessageBody.cpp
    RSMessage.cpp
    SendReceiveToFMOperation.cpp
    ServiceTypeInfo.cpp
    ServiceTypeNotificationReplyMessageBody.cpp
    ServiceTypeNotificationRequestMessageBody.cpp
    UpgradeDescription.cpp
)

add_library(objects_FailoverCommon OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_FailoverCommon Common.Stdafx.h)

add_library(${lib_FailoverCommon} STATIC $<TARGET_OBJECTS:objects_FailoverCommon>)

set_target_properties(${lib_FailoverCommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})       
