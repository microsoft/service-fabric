// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RESOURCE_ID( Base, Index ) (Base+Index)

// ******************************************************************************
// Define localizable string resources below.
//
// All text after the comment "//" gets extracted as the string value.
// Leading whitespace between the "//" and the first non-whitespace character
// is ignored.
//
// As a guideline, reserve 1000 IDs for new components, though you can reserve
// more if necessary. Consider splitting reserved ID ranges among
// smaller sub-components as appropriate.
//
// After we have gone through the localization process:
// Do NOT make resource ID changes that will cause existing IDs to change their
// underlying value.  (i.e. adding/deleting/modifying IDs that will shift their
// index relative to a component)
//
// Do not strip trailing white spaces on each line, the space is used as part of
// the string.
//
// ******************************************************************************


#define IDS_PERFORMANCE_COUNTER_MIN                         RESOURCE_ID( 0, 0 )
#define IDS_PERFORMANCE_COUNTER_MAX                         RESOURCE_ID( 0, 9999 )

#define IDS_COMMON( Index )                                 RESOURCE_ID( 10000, Index )
#define IDS_COMMON_RESERVED1                                IDS_COMMON( 1 ) // Reserved 1: This resource string is reserved for internal testing.
#define IDS_COMMON_RESERVED2                                IDS_COMMON( 2 ) // Reserved 2: There's no problem, everything's going to be fine.
#define IDS_COMMON_Invalid_Percent_Value                    IDS_COMMON( 3 ) // Invalid percent value:
#define IDS_COMMON_String_Too_Long                          IDS_COMMON( 4 ) // String value exceeds maximum allowed length (string, length, max):
#define IDS_COMMON_ENTRY_TOO_LARGE                          IDS_COMMON( 5 ) // Entry is too large. Estimated size and maximum allowed size are:
#define IDS_COMMON_Invalid_Null_Pointer                     IDS_COMMON( 6 ) // Invalid NULL pointer:
#define IDS_COMMON_Invalid_LPCWSTR_Length                   IDS_COMMON( 7 ) // Input string doesn't respect parameter size limits. Parameter name, min and max characters length:
#define IDS_COMMON_Null_Items                               IDS_COMMON( 8 ) // Items is NULL, but Count is
#define IDS_COMMON_Invalid_Continuation_Token               IDS_COMMON( 9 ) // The continuation token '{0}' is not valid. Please pass in the continuation token returned by a previous query.
#define IDS_COMMON_String_Too_Long2                         IDS_COMMON( 10 ) // The string '{0}...' is longer than the maximum allowed length of {1} characters.
#define IDS_COMMON_String_Too_Short                         IDS_COMMON( 11 ) // The string '{0}' is shorter than the minimum allowed length of {1} characters.
#define IDS_COMMON_Not_Primary                              IDS_COMMON( 12 ) // The replica with id {0} is not primary for partition {1}. Please retry.
#define IDS_COMMON_UNIX_Unzip_Failure                       IDS_COMMON( 13 ) // Unzipping failed at {0}, error {1}
#define IDS_COMMON_UNIX_Zip_Failure                         IDS_COMMON( 14 ) // Zipping failed at {0}, error {1}
#define IDS_COMMON_Invalid_FabricVersion                    IDS_COMMON( 15 ) // Invalid cluster manifest version '{0}'. '{1}' character is not allowed.
#define IDS_COMMON_Invalid_Max_Results                      IDS_COMMON( 16 ) // Invalid max results '{0}' passed in. The value must be a positive integer. Max value is the max of int64.
#define IDS_COMMON_Invalid_String_Boolean_Value             IDS_COMMON( 17 ) // The parameter {0} has invalid value '{1}'. Expected true/false.
#define IDS_COMMON_Max_Results_Reached                      IDS_COMMON( 18 ) // Reached max configured results of {0} entries per one page.
#define IDS_COMMON_Reserved_Field_Not_Null                  IDS_COMMON( 19 ) // Internal error: expected NULL Reserved field.
#define IDS_COMMON_Zip_Failed                               IDS_COMMON( 20 ) // TryZipDirectory failed: src={0} dest={1}
#define IDS_COMMON_Unzip_Failed                             IDS_COMMON( 21 ) // TryUnzipDirectory failed: src={0} dest={1}
#define IDS_COMMON_Win_Long_Paths                           IDS_COMMON( 22 ) // Note that long paths exceeding the Windows MAX_PATH limit are not supported for compressed packages.
#define IDS_COMMON_Access_Token_Failed                      IDS_COMMON( 23 ) // GetAccessToken failed: authority={0} cluster={1} client={2} error={3}
#define IDS_COMMON_IsAdmin_Role_Failed                      IDS_COMMON( 24 ) // IsAdminRole failed: issuer={0} audience={1} roleClaim={2} cert={3} error={4}
#define IDS_COMMON_InvalidOperationOnRootNameURI            IDS_COMMON( 25 ) // The operation can't be performed on the root name URI '{0}'.
#define IDS_COMMON_InvalidNameQueryCharacter                IDS_COMMON( 26 ) // The name '{0}' is invalid: character '?' is not supported.
#define IDS_COMMON_InvalidNameServiceGroupCharacter         IDS_COMMON( 27 ) // The name '{0}' is invalid: character '#' is reserved by Service Groups.
#define IDS_COMMON_InvalidNameExceedsMaxSize                IDS_COMMON( 28 ) // The name '{0}' (size {1}) exceeds the max allowed size, {2}.
#define IDS_COMMON_Invalid_Node_Instance                    IDS_COMMON( 29 ) // Invalid node instance id '{0}' passed in. The value must be a uint64.
#define IDS_COMMON_Invalid_Guid                             IDS_COMMON( 30 ) // Invalid value '{0}' passed in. The value must be a GUID.
#define IDS_COMMON_Invalid_Int64                            IDS_COMMON( 31 ) // Invalid value '{0}' passed in. The value must be a int64.
#define IDS_COMMON_FileNotFound                             IDS_COMMON( 32 ) // The file '{0}' doesn't exist.
#define IDS_COMMON_Invalid_Sfpkg_Name                       IDS_COMMON( 33 ) // The file '{0}' doesn't have the required '.sfpkg' extension.
#define IDS_COMMON_ZipToChildDirectoryFails                 IDS_COMMON( 34 ) // Zip to directory '{0}' fails because it's a child of the source directory '{1}'.
#define IDS_COMMON_ExpandSfpkgDirectoryNotEmpty             IDS_COMMON( 35 ) // The directory '{0}' is not empty. Please delete the content or provide another directory as application package root.
#define IDS_COMMON_DirectoryNotFound                        IDS_COMMON( 36 ) // The directory '{0}' doesn't exist.
#define IDS_COMMON_Resource_Name_Mismatch                   IDS_COMMON( 37 ) // {0} name specified in the request Uri {1} doesn't match the name specified in the body {2}.

#define IDS_EXE( Index )                                    RESOURCE_ID( 11000, Index )
#define IDS_FABRICHOST( Index )                             RESOURCE_ID( IDS_EXE( 0 ), Index )
#define IDS_FABRICHOST_STARTING                             IDS_FABRICHOST( 1 ) // Starting Fabric Host as console application.
#define IDS_FABRICHOST_CLOSING                              IDS_FABRICHOST( 2 ) // Closing Fabric Activation Manager.
#define IDS_FABRICHOST_CLOSED                               IDS_FABRICHOST( 3 ) // Fabric Activation Manager closed.
#define IDS_FABRICHOST_EXCEPTION                            IDS_FABRICHOST( 4 ) // Unhandled exception.
#define IDS_FABRICHOST_NTServiceName                        IDS_FABRICHOST( 5 ) // Windows Fabric Distributed Platform Service
#define IDS_FABRICHOST_NTServiceDescription                 IDS_FABRICHOST( 6 ) // This service controls the Windows Fabric process on this machine.

#define IDS_FABRIC_SERVICE( Index )                         RESOURCE_ID( IDS_EXE( 200 ), Index )
#define IDS_FABRIC_SERVICE_FMREADY                          IDS_FABRIC_SERVICE( 1 )  // Failover Manager is ready.
#define IDS_FABRIC_SERVICE_OPENING                          IDS_FABRIC_SERVICE( 2 )  // Opening Fabric node.
#define IDS_FABRIC_SERVICE_REQ_VERSION                      IDS_FABRIC_SERVICE( 3 )  // Requested code version is:
#define IDS_FABRIC_SERVICE_EXE_VERSION                      IDS_FABRIC_SERVICE( 4 )  // Executing code version is:
#define IDS_FABRIC_SERVICE_OPENED                           IDS_FABRIC_SERVICE( 5 )  // Fabric node opened. Waiting for Failover Manager to become ready.
#define IDS_FABRIC_SERVICE_EXIT_PROMPT                      IDS_FABRIC_SERVICE( 6 )  // Press [Enter] to exit.
#define IDS_FABRIC_SERVICE_WARN_SHUTDOWN                    IDS_FABRIC_SERVICE( 7 )  // Shutting down while open is still pending.
#define IDS_FABRIC_SERVICE_CLOSING                          IDS_FABRIC_SERVICE( 8 )  // Closing Fabric node.
#define IDS_FABRIC_SERVICE_CLOSED                           IDS_FABRIC_SERVICE( 9 )  // Fabric node closed.
#define IDS_FABRIC_SERVICE_CTRL_C                           IDS_FABRIC_SERVICE( 10 ) // [Ctrl+C] caught.
#define IDS_FABRIC_SERVICE_CTRL_BRK                         IDS_FABRIC_SERVICE( 11 ) // [Ctrl+Break] caught.

#define IDS_FABRICGATEWAY( Index )                            RESOURCE_ID( IDS_EXE( 400 ), Index )
#define IDS_FABRICGATEWAY_STARTING                            IDS_FABRICGATEWAY( 1 ) // Starting Fabric Gateway.
#define IDS_FABRICGATEWAY_STARTED                             IDS_FABRICGATEWAY( 2 ) // Fabric Gateway started.
#define IDS_FABRICGATEWAY_EXIT_PROMPT                         IDS_FABRICGATEWAY( 3 ) // Press [Enter] to exit.
#define IDS_FABRICGATEWAY_CLOSING                             IDS_FABRICGATEWAY( 4 ) // Closing Fabric Gateway.
#define IDS_FABRICGATEWAY_CLOSED                              IDS_FABRICGATEWAY( 5 ) // Fabric Gateway closed.
#define IDS_FABRICGATEWAY_CTRL_C                              IDS_FABRICGATEWAY( 6 ) // [Ctrl+C] caught.
#define IDS_FABRICGATEWAY_CTRL_BRK                            IDS_FABRICGATEWAY( 7 ) // [Ctrl+Break] caught.
#define IDS_FABRICGATEWAY_WARN_SHUTDOWN                       IDS_FABRICGATEWAY( 8 ) // Shutting down while open is still pending.

#define IDS_FAILOVER( Index )                               RESOURCE_ID( 12000, Index )
#define IDS_FAILOVER_Node_Up                                IDS_FAILOVER( 1 )  // Fabric node is up.
#define IDS_FAILOVER_Node_Down                              IDS_FAILOVER( 2 )  // Fabric node is down. For more information see: 
#define IDS_FAILOVER_Node_Down_During_Upgrade               IDS_FAILOVER( 3 )  // Fabric node is taking longer than expected to complete upgrade.
#define IDS_FAILOVER_Partition_Healthy                      IDS_FAILOVER( 4 )  // Partition is healthy.
#define IDS_FAILOVER_Partition_PlacementStuck               IDS_FAILOVER( 5 )  // Partition is below target replica or instance count.
#define IDS_FAILOVER_Partition_BuildStuck                   IDS_FAILOVER( 6 )  // Replicas are taking longer than expected to build on the following Fabric nodes:
#define IDS_FAILOVER_Partition_ReconfigurationStuck         IDS_FAILOVER( 7 )  // Partition reconfiguration is taking longer than expected.
#define IDS_FAILOVER_Partition_QuorumLoss                   IDS_FAILOVER( 8 )  // Partition is in quorum loss.
#define IDS_FAILOVER_Service_Created                        IDS_FAILOVER( 9 )  // Service has been created.
#define IDS_FAILOVER_Deletion_InProgress                    IDS_FAILOVER( 10 )  // Deletion in progress.
#define IDS_FAILOVER_Invalid_Node_Upgrade_Phase             IDS_FAILOVER( 11 )  // Invalid node upgrade phase:
#define IDS_FAILOVER_Partition_RebuildStuck                 IDS_FAILOVER( 12 )  // Partition rebuild is taking longer than expected.
#define IDS_FAILOVER_Node_DeactivateStuck                   IDS_FAILOVER( 13 )  // Fabric node is taking longer than expected to deactivate.
#define IDS_FAILOVER_Seed_Node_Down                         IDS_FAILOVER( 14 )  // Fabric seed node is down. Loss of a majority of seed nodes can cause cluster failure. For more information see: 
#define IDS_FAILOVER_ServiceDescription_InitializationData_Size_Changed                   IDS_FAILOVER( 15 )  // Service initialization data size changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_InitializationData_Changed                   IDS_FAILOVER( 16 )  // {0}-th byte of service initialization data changed from '{1}' to '{2}'
#define IDS_FAILOVER_ServiceDescription_Load_Metric_Description_Count_Changed                   IDS_FAILOVER( 17 )  // Service load metric description count changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_Load_Metric_Removed                   IDS_FAILOVER( 18 )  // Service load metric description '{0}' was removed
#define IDS_FAILOVER_ServiceDescription_Load_Metric_Added                   IDS_FAILOVER( 19 )  // Service load metric description '{0}' was added
#define IDS_FAILOVER_ServiceDescription_ServiceName_Changed                   IDS_FAILOVER( 20 )  // Service name changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ServiceType_Changed                   IDS_FAILOVER( 21 )  // Service type changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ApplicationName_Changed                   IDS_FAILOVER( 22 )  // Application name changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_Partition_Count_Changed                   IDS_FAILOVER( 23 )  // Partition count changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_TargetReplicaSetSize_Changed                   IDS_FAILOVER( 24 )  // TargetReplicaSetSize has changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_Changed_To_Stateful                   IDS_FAILOVER( 25 )  // Service changed from being stateless to stateful
#define IDS_FAILOVER_ServiceDescription_Changed_To_Stateless                   IDS_FAILOVER( 26 )  // Service changed from being stateful to stateless
#define IDS_FAILOVER_ServiceDescription_Changed_To_Persisted                   IDS_FAILOVER( 27 )  // Service changed from being non-persisted to persisted
#define IDS_FAILOVER_ServiceDescription_Changed_To_NonPersisted                   IDS_FAILOVER( 28 )  // Service changed from being persisted to non-persisted
#define IDS_FAILOVER_ServiceDescription_ReplicaRestartWaitDuration_Changed                   IDS_FAILOVER( 29 )  // ReplicaRestartWaitDuration changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_QuorumLossWaitDuration_Changed                   IDS_FAILOVER( 30 )  // QuorumLossWaitDuration changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_StandByReplicaKeepDuration_Changed                   IDS_FAILOVER( 31 )  // StandByReplicaKeepDuration changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_PlacementConstraints_Changed                   IDS_FAILOVER( 32 )  // PlacementConstraints changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_PlacementPolicy_Count_Changed                   IDS_FAILOVER( 33 )  // PlacementPolicy count changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ScaleoutCount_Changed                   IDS_FAILOVER( 34 )  // ScaleoutCount changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_DefaultMoveCost_Changed                   IDS_FAILOVER( 35 )  // DefaultMoveCost changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ServicePackageActivationMode_Changed                   IDS_FAILOVER( 36 )  // ServicePackageActivationMode changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ServiceDnsName_Changed                   IDS_FAILOVER( 37 )  // ServiceDnsName changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_PackageVersionInstance_Changed                   IDS_FAILOVER( 38 )  // PackageVersionInstance changed from '{0}' to '{1}'
#define IDS_FAILOVER_ServiceDescription_ScalingPolicies_Size_Changed                   IDS_FAILOVER( 39 )  // Scaling policy count changed from '{0}' to '{1}' in ServiceDescription
#define IDS_FAILOVER_ServiceDescription_MinReplicaSetSize_Changed                   IDS_FAILOVER( 40 )  // MinReplicaSetSize changed from '{0}' to '{1}' in ServiceDescription
#define IDS_FAILOVER_ServiceDescription_Changed_To_Lone_Service                   IDS_FAILOVER( 41 )  // Service is no longer a member of a ServiceGroup
#define IDS_FAILOVER_ServiceDescription_Changed_To_ServiceGroup                   IDS_FAILOVER( 42 )  // Service has become a member of a ServiceGroup
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingTrigger_Changed                   IDS_FAILOVER( 43 )  // Scaling trigger has changed from '{0}' to '{1}' in ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingTrigger_Removed                   IDS_FAILOVER( 44 )  // Scaling trigger '{0}' was removed from ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingTrigger_Added                   IDS_FAILOVER( 45 )  // Scaling trigger '{0}' was added to ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingMechanism_Changed                   IDS_FAILOVER( 46 )  // Scaling mechanism changed from '{0}' to '{1}' in ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingMechanism_Removed                   IDS_FAILOVER( 47 )  // Scaling mechanism '{0}' was removed from ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServiceScalingPolicyDescription_ScalingMechanism_Added                   IDS_FAILOVER( 48 )  // Scaling mechanism '{0}' was added to ServiceScalingPolicyDescription
#define IDS_FAILOVER_ServicePlacementPolicyDescription_DomainName_Changed                   IDS_FAILOVER( 49 )  // Domain name changed from '{0}' to '{1}' in ServicePlacementPolicyDescription
#define IDS_FAILOVER_ServicePlacementPolicyDescription_Type_Changed                   IDS_FAILOVER( 50 )  // Description type changed from '{0}' to '{1}' in ServicePlacementPolicyDescription
#define IDS_FAILOVER_ServiceGroupMemberLoadMetricDescription_Name_Changed                   IDS_FAILOVER( 51 )  // Name changed from '{0}' to '{1}' in ServiceGroupMemberLoadMetricDescription
#define IDS_FAILOVER_ServiceGroupMemberLoadMetricDescription_Weight_Changed                   IDS_FAILOVER( 52 )  // Weight changed from '{0}' to '{1}' in ServiceGroupMemberLoadMetricDescription
#define IDS_FAILOVER_ServiceGroupMemberLoadMetricDescription_PrimaryDefaultLoad_Changed                   IDS_FAILOVER( 53 )  // PrimaryDefaultLoad changed from '{0}' to '{1}' in ServiceGroupMemberLoadMetricDescription
#define IDS_FAILOVER_ServiceGroupMemberLoadMetricDescription_SecondaryDefaultLoad_Changed                   IDS_FAILOVER( 54 )  // SecondaryDefaultLoad changed from '{0}' to '{1}' in ServiceGroupMemberLoadMetricDescription
#define IDS_FAILOVER_ServiceGroupMemberDescription_Member_Identifier_Changed                   IDS_FAILOVER( 55 )  // Member identifier changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupMemberDescription_ServiceDescriptionType_Changed                   IDS_FAILOVER( 56 )  // ServiceDescriptionType changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupMemberDescription_ServiceType_Changed                   IDS_FAILOVER( 57 )  // ServiceType changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupMemberDescription_ServiceName_Changed                   IDS_FAILOVER( 58 )  // ServiceName changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupMemberDescription_Load_Metric_Count_Changed                   IDS_FAILOVER( 59 )  // Count of load metrics changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupDescription_ServiceGroupInitializationData_Size_Changed                   IDS_FAILOVER( 60 )  // Size of ServiceGroupInitializationData changed from '{0}' to '{1}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupDescription_ServiceGroupInitializationData_Changed                   IDS_FAILOVER( 61 )  // {0}-th byte of ServiceGroupInitializationData changed from '{1}' to '{2}' in ServiceGroupMemberDescription
#define IDS_FAILOVER_ServiceGroupDescription_Changed_To_NonPersisted                  IDS_FAILOVER( 62 )  // ServiceGroupDescription changed from persisted to non-Persisted
#define IDS_FAILOVER_ServiceGroupDescription_Changed_To_Persisted                  IDS_FAILOVER( 63 )  // ServiceGroupDescription changed from non-persisted to persisted
#define IDS_FAILOVER_ServiceGroupDescription_ServiceGroupMemberData_Size_Changed                  IDS_FAILOVER( 64 )  // Size of ServiceGroupMemberData changed from '{0}' to '{1}' in ServiceGroupDescription


#define IDS_FM( Index )                                     RESOURCE_ID( 27000, Index )
#define IDS_FM_Rebuild_Stuck                                IDS_FM( 1 ) // FM rebuild is taking longer than expected. 
#define IDS_FM_Rebuild_Healthy                              IDS_FM( 2 ) // FM rebuild is healthy.
#define IDS_FM_Rebuild_Stuck_Broadcast                      IDS_FM( 3 ) // Node {0} is waiting for broadcast reply from other nodes
#define IDS_FM_Rebuild_Stuck_Nodes                          IDS_FM( 4 ) // Node {0} is waiting for response from nodes:
#define IDS_FM_Rebuild_More_Info                            IDS_FM( 5 ) // . For more information see: {0}
#define IDS_FM_Rebuild_Time_Info                            IDS_FM( 6 ) // . Rebuild elapsed time: {0}. Rebuild expected time: {1}

#define IDS_FMM( Index )                                    RESOURCE_ID( 28000, Index )
#define IDS_FMM_Rebuild_Stuck                               IDS_FMM( 1 ) // FMM rebuild is taking longer than expected. 
#define IDS_FMM_Rebuild_Healthy                             IDS_FMM( 2 ) // FMM rebuild is healthy.

#define IDS_RA( Index )                                     RESOURCE_ID( 13000, Index )
#define IDS_RA_HEALTH_REPLICA_CREATED                       IDS_RA( 1 ) // Replica has been created on
#define IDS_RA_HEALTH_REPLICA_OPEN_STATUS_WARNING           IDS_RA( 2 ) // Replica had multiple failures during open on
#define IDS_RA_HEALTH_REPLICA_OPEN_STATUS_HEALTHY           IDS_RA( 3 ) // Replica has opened successfully.
#define IDS_RA_HEALTH_REPLICA_CHANGEROLE_STATUS_ERROR       IDS_RA( 4 ) // Replica had multiple failures during change role on
#define IDS_RA_HEALTH_REPLICA_CHANGEROLE_STATUS_HEALTHY     IDS_RA( 5 ) // Replica status is healthy.
#define IDS_RA_HEALTH_REPLICA_CLOSE_STATUS_WARNING          IDS_RA( 6 ) // Replica had multiple failures during close on
#define IDS_RA_HEALTH_REPLICA_CLOSE_STATUS_HEALTHY          IDS_RA( 7 ) // Replica has closed successfully.
#define IDS_RA_HEALTH_REPLICA_STR_STATUS_WARNING            IDS_RA( 8 ) // Replica had multiple failures during launching application host.
#define IDS_RA_HEALTH_REPLICA_STR_STATUS_HEALTHY            IDS_RA( 9 ) // Replica has service type registered successfully.
#define IDS_RA_HEALTH_RECONFIGURATION_STATUS_WARNING        IDS_RA( 10 ) // Reconfiguration is stuck.
#define IDS_RA_HEALTH_RECONFIGURATION_STATUS_HEALTHY        IDS_RA( 11 ) // Reconfiguration is healthy.
#define IDS_RA_STORE_PROVIDER_HEALTHY                       IDS_RA( 12 ) // Store provider type {0} created and opened successfully.
#define IDS_RA_STORE_PROVIDER_UNHEALTHY                     IDS_RA( 13 ) // Store provider type {0} creation failed: {1}
#define IDS_RA_STORE_ESE_MIGRATION_BLOCKED                  IDS_RA( 14 ) // Config setting AllowLocalStoreMigration is disabled and TStore database {0} exists
#define IDS_RA_STORE_TSTORE_MIGRATION_BLOCKED               IDS_RA( 15 ) // Config setting AllowLocalStoreMigration is disabled and ESE database {0} exists

#define IDS_RAP( Index )                                    RESOURCE_ID( IDS_RA( 200 ), Index )
#define IDS_RAP_HEALTH_START_TIME                           IDS_RAP( 1 ) // The api

#define IDS_NAMING( Index )                                 RESOURCE_ID( 14000, Index )
#define IDS_NAMING_Invalid_Client_Timeout                   IDS_NAMING( 1 ) // The timeout specified for client APIs must be greater than zero
#define IDS_NAMING_Invalid_Uri                              IDS_NAMING( 2 ) // Failed to parse the requested Naming URI:
#define IDS_NAMING_Invalid_Placement                        IDS_NAMING( 3 ) // Failed to parse the requested Placement Constraints:
#define IDS_NAMING_Invalid_Partition_Count                  IDS_NAMING( 4 ) // Partition count must be at least 1. Requested value is:
#define IDS_NAMING_Invalid_Target_Replicas                  IDS_NAMING( 5 ) // Target replica set size must be at least 1. Requested value is:
#define IDS_NAMING_Invalid_Singleton                        IDS_NAMING( 6 ) // Singleton partition count must be 1. Requested value is:
#define IDS_NAMING_Invalid_Range                            IDS_NAMING( 7 ) // Partition range low key must be less than or equal to the high key. Requested range [low, high] is:
#define IDS_NAMING_Invalid_Range_Count                      IDS_NAMING( 8 ) // Partition range must be greater than or equal to the partition count. Requested range [low, high] and count are:
#define IDS_NAMING_Invalid_Names_Count                      IDS_NAMING( 9 ) // Partition names must be equal in length to the partition count. Requested list of names (...) and count are:
#define IDS_NAMING_Invalid_Partition_Names                  IDS_NAMING( 10 ) // Partition names must be a non-empty list of unique values. Requested list of names = {0}.
#define IDS_NAMING_Invalid_Minimum_Replicas                 IDS_NAMING( 11 ) // Minimum replica set size must be at least 1. Requested value is:
#define IDS_NAMING_Invalid_Minimum_Target                   IDS_NAMING( 12 ) // Minimum replica set size must be less than or equal to the target replica set size. Requested minimum and target values (minimum, target) are:
#define IDS_NAMING_Invalid_Metric                           IDS_NAMING( 13 ) // Service metric name must be a non-empty string.
#define IDS_NAMING_Invalid_Metric_Range                     IDS_NAMING( 14 ) // Service metric weight must be within the supported range. Supported range [low, high] and requested metric (name, value) are:
#define IDS_NAMING_Invalid_Metric_Load                      IDS_NAMING( 15 ) // Service metric for stateless service must have a secondary default load less than or equal to zero. Request metric (name, value) is:
#define IDS_NAMING_Invalid_Affinity_Count                   IDS_NAMING( 16 ) // More than one service affinity is not supported yet.
#define IDS_NAMING_Invalid_Correlation_Scheme               IDS_NAMING( 17 ) // Invalid correlation scheme:
#define IDS_NAMING_Invalid_Correlation_Service              IDS_NAMING( 18 ) // Invalid correlation service name:
#define IDS_NAMING_Invalid_Correlation                      IDS_NAMING( 19 ) // Services with -1 instance count cannot be affinitized to other services. Requested correlation (current, target):
#define IDS_NAMING_Missing_Application_Name                 IDS_NAMING( 20 ) // Managed Service Type is missing application name:
#define IDS_NAMING_Missing_Parameters                       IDS_NAMING( 21 ) // At least one of the required parameters are not specified. Parameter names:
#define IDS_NAMING_Duplicate_Notification_Filter_Name       IDS_NAMING( 22 ) // Service notification filter already registered at name:
#define IDS_NAMING_Invalid_Service_Kind                     IDS_NAMING( 23 ) // Invalid service kind:
#define IDS_NAMING_Operation_Slow                           IDS_NAMING( 24 ) // The {0} started at {1} is taking longer than {2}.
#define IDS_NAMING_Operation_Slow_Completed                 IDS_NAMING( 25 ) // The {0} started at {1} completed with {2} in more than {3}.
#define IDS_NAMING_Invalid_FileStoreClient_Timeout          IDS_NAMING( 26 ) // File store operation timeout must be less than or equal to the cluster configuration for MaxFileOperationTimeout. Requested value and timeout limit:
#define IDS_NAMING_Not_Primary_RecoveryInProgress           IDS_NAMING( 27 ) // The Naming Store Service primary replica with id {0} for partition {1} is executing recovery operations and can not accept user operations yet. Please retry.
#define IDS_NAMING_JobQueueFull                             IDS_NAMING( 28 ) // The Naming Store Service primary replica with id {0} for partition {1} cannot process the request because it is too busy. Please retry.
#define IDS_NAMING_Update_Service_Kind                      IDS_NAMING( 29 ) // Service kind can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_ServicePackageActivationMode      IDS_NAMING( 30 ) // Service 'ServicePackageActivationMode' can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_ServiceDnsName                    IDS_NAMING( 31 ) // Service 'ServiceDnsName' can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_Partition_Count                   IDS_NAMING( 32 ) // Partition count can be updated only for services using NamedPartitioningScheme, for details about different partitioning schemes please refer to the https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-concepts-partitioning. Updated service: {0}.
#define IDS_NAMING_Update_Service_Has_Persisted_State               IDS_NAMING( 33 ) // Service 'HasPersistedState' can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_Is_Service_Group                  IDS_NAMING( 34 ) // Service 'IsServiceGroup' can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_Partition_Scheme                  IDS_NAMING( 35 ) // Service partition scheme can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_Partition_Range                   IDS_NAMING( 36 ) // Service partition range can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Service_Partition_Name                    IDS_NAMING( 37 ) // Service partition name can not be updated. Updated service: {0}.
#define IDS_NAMING_Update_Unsupported_Repartition                   IDS_NAMING( 38 ) // System service repartitioning is not supported.
#define IDS_NAMING_Update_Unsupported_Scheme                        IDS_NAMING( 39 ) // Unsupported partition scheme '{0}' for service update.
#define IDS_NAMING_Update_Unsupported_AddRemove                     IDS_NAMING( 40 ) // Both adding and removing partitions in the same update is not supported for service '{0}'. Please perform two separate updates.
#define IDS_NAMING_Named_Partition_Not_Found                        IDS_NAMING( 41 ) // Named partition '{0}' not found.
#define IDS_NAMING_Exceeded_Max_Partition_Count                     IDS_NAMING( 42 ) // Exceeded maximum partition count of '{0}': count={1}.
#define IDS_NAMING_ScalingPolicy_Scaling_Count                      IDS_NAMING( 43 ) // Only one scaling policy per service is supported. Specified count: {0}.
#define IDS_NAMING_ScalingPolicy_Named_Partitions                   IDS_NAMING( 44 ) // Scaling on the number of partitions is only supported for named partitions. Expected format of names is '0', '1', ...'N-1'.
#define IDS_NAMING_ScalingPolicy_Instance_Count                     IDS_NAMING( 45 ) // Scaling on the number of instances is only supported for stateless services.
#define IDS_NAMING_ScalingPolicy_Metric_Name                        IDS_NAMING( 46 ) // Metric {0} specified for scaling is neither a resource nor a metric of the service.
#define IDS_NAMING_ScalingPolicy_Partitions_Scaling                 IDS_NAMING( 47 ) // Scaling the number of partitions is only supported with average service load.
#define IDS_NAMING_ScalingPolicy_Instances_Scaling                  IDS_NAMING( 48 ) // Scaling the number of instances is only supported with average partition load.
#define IDS_NAMING_ScalingPolicy_Increment                          IDS_NAMING( 49 ) // Scaling increment should be greater than 0.
#define IDS_NAMING_ScalingPolicy_MinMaxPartitions                   IDS_NAMING( 50 ) // Minimum number of partitions {0} cannot be greater than maximum number of partitions {1}.
#define IDS_NAMING_ScalingPolicy_MinMaxInstances                    IDS_NAMING( 51 ) // Minimum number of instances {0} cannot be greater than maximum number of instances {1}.
#define IDS_NAMING_ScalingPolicy_Threshold                          IDS_NAMING( 52 ) // Lower load threshold {0} cannot be greater than upper load threshold {1}.
#define IDS_NAMING_ScalingPolicy_LowerLoadThreshold                 IDS_NAMING( 53 ) // Lower load threshold {0} must be greater than or equal to zero.
#define IDS_NAMING_ScalingPolicy_UpperLoadThreshold                 IDS_NAMING( 54 ) // Upper load threshold {0} must be greater than zero.
#define IDS_NAMING_ScalingPolicy_MinInstanceCount                   IDS_NAMING( 55 ) // Minimum instance count {0} must be greater than or equal to zero.
#define IDS_NAMING_ScalingPolicy_MaxInstanceCount                   IDS_NAMING( 56 ) // Maximum instance count {0} must be either equal to -1 (unlimited) or greater than zero.
#define IDS_NAMING_ScalingPolicy_MinPartitionCount                  IDS_NAMING( 57 ) // Minimum partition count {0} must be greater than or equal to zero.
#define IDS_NAMING_ScalingPolicy_MaxPartitionCount                  IDS_NAMING( 58 ) // Maximum partition count {0} must be either equal to -1 (unlimited) or greater than zero.
#define IDS_NAMING_Invalid_Flags                                    IDS_NAMING( 59 ) // Invalid flags: 0x{0:x}
#define IDS_NAMING_ScalingPolicy_UseOnlyPrimaryLoad                 IDS_NAMING( 60 ) // Use of primary load for auto scaling is allowed only for stateful services.
#define IDS_NAMING_ScalingPolicy_ScaleIntervalInSeconds             IDS_NAMING( 61 ) // Scaling interval set to: {0} seconds, must be greater than zero seconds.
#define IDS_NAMING_PartitionName_Count_Changed                      IDS_NAMING( 62 ) // Partition names count changed from '{0}' to '{1}'.
#define IDS_NAMING_PartitionName_Removed                            IDS_NAMING( 63 ) // Partition name '{0}' has been removed.
#define IDS_NAMING_PartitionName_Inserted                           IDS_NAMING( 64 ) // Partition name '{0}' has been inserted.
#define IDS_NAMING_PartitionScheme_Changed                          IDS_NAMING( 65 ) // Partition scheme changed from '{0}' to '{1}'.
#define IDS_NAMING_PSD_Version_Changed                              IDS_NAMING( 66 ) // PSD version changed from '{0}' to '{1}'.
#define IDS_NAMING_PSD_LowRange_Changed                             IDS_NAMING( 67 ) // PSD low range changed from '{0}' to '{1}'.
#define IDS_NAMING_PSD_HighRange_Changed                            IDS_NAMING( 68 ) // PSD high range changed from '{0}' to '{1}'.

#define IDS_STORE( Index )                                  RESOURCE_ID( IDS_NAMING( 500 ), Index )
#define IDS_STORE_Slow_Commit                               IDS_STORE( 1 ) // Slow commits detected: count={0} time=[{1}, {2}]
#define IDS_STORE_Path_Too_Long                             IDS_STORE( 2 ) // Database directory path too long: path={0}
#define IDS_STORE_Drain_Store                               IDS_STORE( 3 ) // Started draining current local database at {0}
#define IDS_STORE_FS_FullCopy_Unzip                         IDS_STORE( 5 ) // Started decompressing physical full copy database files at {0} (compressed={1} bytes)
#define IDS_STORE_FS_FullCopy_Restore                       IDS_STORE( 6 ) // Started restoring physical full copy database files at {0}
#define IDS_STORE_FullCopy_Start                            IDS_STORE( 7 ) // Started swapping logical full copy database at {0}
#define IDS_STORE_PartialCopy_Start                         IDS_STORE( 8 ) // Started replaying logical partial copy operations at {0}
#define IDS_STORE_Open_Failed                               IDS_STORE( 9 ) // Open failed with error {0}
#define IDS_STORE_AutoCompaction                            IDS_STORE( 10 ) // Database file size ({0} bytes) exceeds {1}MB. Open may take longer than usual as auto-compaction runs.
#define IDS_STORE_Large_Database_File                       IDS_STORE( 11 ) // Database file size ({0} bytes) exceeds {1}MB and could not be auto-compacted to a smaller size.
#define IDS_STORE_Restore_Dir_Empty                         IDS_STORE( 12 ) // Supplied restore directory [{0}] is empty.
#define IDS_STORE_BackupChainId_Mismatch                    IDS_STORE( 13 ) // BackupChainId mismatch for backup under [{0}]. Actual: [{1}], Expected: [{2}].
#define IDS_STORE_Duplicate_Backups                         IDS_STORE( 14 ) // Subdirectories [{0}] and [{1}] contain duplicate backups. BackupChainId: [{2}], BackupIndex: [{3}].
#define IDS_STORE_Invalid_BackupChain                       IDS_STORE( 15 ) // Specified restore folder {0} contains broken backup chain. Missing backup index: {1}, Existing backup indexes: {2}.
#define IDS_STORE_BackupInProgress_By_LogTruncation         IDS_STORE( 16 ) // A backup initiated by internal system log truncation is in progress. Please retry after sometime.
#define IDS_STORE_BackupInProgress_By_System                IDS_STORE( 17 ) // A backup initiated by an internal system operation is in progress. Please retry after sometime.
#define IDS_STORE_BackupInProgress_By_User                  IDS_STORE( 18 ) // A previously initiated backup by user is currently in progress.
#define IDS_STORE_MissingBackupFile                         IDS_STORE( 19 ) // Expected backup file(s) [{0}] are missing from restore folder [{1}].
#define IDS_STORE_ExtraBackupFile                           IDS_STORE( 20 ) // Extra file(s) [{0}] not part of original backup is present in restore folder [{1}].
#define IDS_STORE_InvalidRestoreData                        IDS_STORE( 21 ) // Restore metadata in file [{0}] has invalid '{1}'. Restore Metadata: [{2}].
#define IDS_STORE_RestoreDataFileMissing                    IDS_STORE( 22 ) // Restore data file [{0}] not found.
#define IDS_STORE_RestorePartitionInfoMismatch              IDS_STORE( 23 ) // Restore metadata partition info mismatch: current = [{0}], restore = [{1}], MetadataFile = [{2}].
#define IDS_STORE_RestoreLsnCheckFailed                     IDS_STORE( 24 ) // Backup provided for restore has older data than present in service. RestoreSequenceNumber=[{0}], CurrentSequenceNumber=[{1}].
#define IDS_STORE_ESE_Error                                 IDS_STORE( 25 ) // Encountered ESE error: {0}
#define IDS_STORE_MigrationBatchProgress                    IDS_STORE( 26 ) // completed migration batch: type='{0}' key='{1}' error={2} count=[{3} batch, {4} total] done={5}
#define IDS_STORE_MigrationMirrorCommitFailed               IDS_STORE( 27 ) // failed to mirror commit
#define IDS_STORE_MigrationDestTxLookupFailed               IDS_STORE( 28 ) // failed to find destination transaction: src={0}
#define IDS_STORE_MigrationRestoreInterruption              IDS_STORE( 29 ) // cannot interrupt migration in processing state with restore
#define IDS_STORE_MigrationPhaseFailed                      IDS_STORE( 30 ) // migration phase {0} failed: {1} ({2})
#define IDS_STORE_MigrationSourceTxExists                   IDS_STORE( 31 ) // source tx already exists: {0}
#define IDS_STORE_MigrationCreateTxFailed                   IDS_STORE( 32 ) // failed to create target store transaction: {0}
#define IDS_STORE_MigrationLocalBackup                      IDS_STORE( 33 ) // backing up source store to '{0}'
#define IDS_STORE_MigrationArchiveBackup                    IDS_STORE( 34 ) // archiving source store backup to '{0}'
#define IDS_STORE_MigrationArchiveUpload                    IDS_STORE( 35 ) // uploading archived backup to '{0}'

#define IDS_MANAGEMENT( Index )                                         RESOURCE_ID( 15000, Index )
#define IDS_CM( Index )                                                 RESOURCE_ID( IDS_MANAGEMENT( 0 ), Index )
#define IDS_CM_Application_Created                                      IDS_CM( 1 ) // Application has been created.
#define IDS_CM_Application_Updated                                      IDS_CM( 2 ) // Application health policy has been updated.
#define IDS_CM_AppTypeName_Too_Long                                     IDS_CM( 3 ) // Application type name must not exceed length limit. Requested name and length limit:
#define IDS_CM_AppTypeVersion_Too_Long                                  IDS_CM( 4 ) // Application type version must not exceed length limit. Requested version and length limit:
#define IDS_CM_Invalid_Service_Prefix                                   IDS_CM( 5 ) // Application name must be a prefix of service name. Requested application and service name:
#define IDS_CM_Invalid_Service_Name                                     IDS_CM( 6 ) // Application name must not equal service name. Requested service name:
#define IDS_CM_Invalid_Upgrade_Mode                                     IDS_CM( 7 ) // Upgrade mode must be unmonitored manual. Current mode:
#define IDS_CM_Invalid_Upgrade_Instance                                 IDS_CM( 8 ) // Upgrade instance must match current upgrade instance. Requested target and current instances:
#define IDS_CM_Invalid_Upgrade_Domain                                   IDS_CM( 9 ) // Requested upgrade domain target '{0}' does not match the next expected upgrade domain '{1}'. Current in-progress='{2}'.
#define IDS_CM_Invalid_Fabric_Version                                   IDS_CM( 10 ) // Must provision either a valid Fabric code or config version. Requested value:
#define IDS_CM_Invalid_First_Upgrade                                    IDS_CM( 11 ) // The first Fabric upgrade must specify both the code and config versions. Requested value:
#define IDS_CM_Invalid_Fabric_Upgrade                                   IDS_CM( 12 ) // Must specify either a valid target code or config version for Fabric upgrade. Requested value:
#define IDS_CM_ServiceType_Removal                                      IDS_CM( 13 ) // Services must be explicitly deleted before removing their Service Types. Removed Service Type:
#define IDS_CM_ServiceType_Move                                         IDS_CM( 14 ) // Services must be explicitly deleted before moving their Service Types between packages. Moved Service Type:
#define IDS_CM_Default_ServiceType                                      IDS_CM( 15 ) // Service Type must exist before creating a default service. Requested default service and Service Type:
#define IDS_CM_Default_Service_Description                              IDS_CM( 16 ) // The default service description for '{0}' was modified as part of upgrade, which is not allowed. {1}. To allow the modification, set EnableDefaultServicesUpgrade to true following https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings. Note, at the start of the application upgrade, there may be three descriptions of a default service: (1) the description with which it was last created (2) the description with which it is currently running, which could diverge from (1) because of Update-ServiceFabricService API call and (3) the description in the target manifest. This error is about the difference between the descriptions (1) and (3). For more details, refer to https://aka.ms/upgrade-defaultservices
#define IDS_CM_Invalid_Upgrade_Failure_Action                           IDS_CM( 17 ) // Invalid upgrade failure action.
#define IDS_CM_Invalid_Overall_Timeout                                  IDS_CM( 18 ) // Overall upgrade timeout must be greater than the combined health check wait and retry timeouts. Requested values (overall timeout, wait, retry):
#define IDS_CM_Invalid_Domain_Timeout                                   IDS_CM( 19 ) // Upgrade domain timeout must be greater than the combined health check wait and retry timeouts. Requested values (domain timeout, wait, retry):
#define IDS_CM_Invalid_Overall_Timeout2                                 IDS_CM( 20 ) // Overall upgrade timeout must be greater than or equal to the upgrade domain timeout. Requested values (overall timeout, upgrade domain timeout):
#define IDS_CM_Invalid_Upgrade_Kind                                     IDS_CM( 21 ) // Invalid upgrade kind:
#define IDS_CM_Invalid_Flags                                            IDS_CM( 22 ) // Invalid flags:
#define IDS_CM_Invalid_Null_Pointer                                     IDS_CM( 23 ) // Invalid NULL pointer:
#define IDS_CM_Invalid_Upgrade_Mode2                                    IDS_CM( 24 ) // Invalid upgrade mode:
#define IDS_CM_Mismatched_Application                                   IDS_CM( 25 ) // Application name does not match: requested='{0}' actual='{1}'
#define IDS_CM_No_Updates                                               IDS_CM( 26 ) // Must specify an upgrade parameter to update
#define IDS_CM_Invalid_Upgrade_State                                    IDS_CM( 27 ) // Invalid upgrade state:
#define IDS_CM_Allowed_Rollback_Updates                                 IDS_CM( 28 ) // Only ReplicaSetCheckTimeout, ForceRestart, and RollingUpgradeMode UnmonitoredManual/UnmonitoredAuto may be updated during rollback. Current state:
#define IDS_CM_Invalid_Domain_Timeout2                                  IDS_CM( 29 ) // Upgrade domain timeout must be greater than the combined health check wait and stable durations. Requested values (domain timeout, wait, stable):
#define IDS_CM_Invalid_Overall_Timeout3                                 IDS_CM( 30 ) // Overall upgrade timeout must be greater than the combined health check wait and stable durations. Requested values (overall timeout, wait, stable):
#define IDS_CM_Invalid_Fabric_Code_Version                              IDS_CM( 31 ) // Fabric Code version must be in the format 'major.minor.build.hotfix'. Requested version:
#define IDS_CM_Invalid_Fabric_Config_Version                            IDS_CM( 32 ) // Fabric Config version must be a non-empty string and cannot contain ':'. Requested version:
#define IDS_CM_Preparing_Upgrade                                        IDS_CM( 33 ) // Service creation blocked while validating and preparing application upgrade:
#define IDS_CM_Blocked_Service_Type                                     IDS_CM( 34 ) // Service Type is either being removed or moved between packages during application upgrade:
#define IDS_CM_Mismatched_Application_Type                              IDS_CM( 35 ) // Mismatched application type names (current, incoming):
#define IDS_CM_Mismatched_Application_Version                           IDS_CM( 36 ) // Mismatched application type versions (current, incoming):
#define IDS_CM_Operation_Failed                                         IDS_CM( 37 ) // Creation or deletion was terminated due to persistent failures
#define IDS_CM_ApplicationType_Parse                                    IDS_CM( 38 ) // Failed to parse application type and version from '{0}'. Note that ':' is not supported in the application type or version strings.
#define IDS_CM_ImageBuilder_Running                                     IDS_CM( 39 ) // Running Image Builder process ...
#define IDS_CM_Validate_Update_Default_Service_Fail                     IDS_CM( 40 ) // Default service update validation failed: {0}
#define IDS_CM_Upgrade_Load_Service_Description                         IDS_CM( 41 ) // Failed to load active default service descriptions due to {0}
#define IDS_CM_Update_Default_Service                                   IDS_CM( 42 ) // Updating default service(s)
#define IDS_CM_Update_Default_Service_Failed                            IDS_CM( 43 ) // Update default service failed during roll forward due to {0}.
#define IDS_CM_Update_Default_Service_Rollback_Failed                   IDS_CM( 44 ) // Update default service failed during roll back due to {0}.
#define IDS_CM_Create_Default_Service                                   IDS_CM( 45 ) // Creating default service(s)
#define IDS_CM_Delete_Default_Service                                   IDS_CM( 46 ) // Deleting default service(s)
#define IDS_CM_ApplicationTypeAlreadyProvisioned                        IDS_CM( 47 ) // Application type '{0}:{1}' already provisioned with different casing '{2}:{3}'.
#define IDS_CM_Update_Default_Service_ServicePackageActivationMode      IDS_CM( 48 ) // Default service 'ServicePackageActivationMode' can not be modified. Modified default services: {0}.
#define IDS_CM_Application_Filter_Dup_Defined                           IDS_CM( 49 ) // At most one of ApplicationName, ApplicationTypeName, or ApplicationDefinitionKindFilter can be specified.
#define IDS_CM_Service_Name_Type_Dup_Defined                            IDS_CM( 50 ) // Can not have both ServiceName and ServiceTypeName specified.
#define IDS_CM_Update_Default_Service_ServiceDnsName                    IDS_CM( 51 ) // Default service 'ServiceDnsName' can not be modified. Modified default services: {0}.
#define IDS_CM_AppName_Too_Long                                         IDS_CM( 52 ) // Application name '{0}' must not exceed the length limit of {1} characters.
#define IDS_CM_Compose_App_Provisioning_Failed                          IDS_CM( 53 ) // Provisioning failed due to '{0}' : {1}.
#define IDS_CM_Compose_App_Creation_Failed                              IDS_CM( 54 ) // Application creation failed due to '{0}' : {1}.
#define IDS_CM_Dns_Name_In_Use                                          IDS_CM( 55 ) // Service {0} creation failed due to DNS name {1} is in use.
#define IDS_CM_Version_Given_Without_Type                               IDS_CM( 56 ) // Query {0} cannot have a version filter without a name filter. Version provided is {1}.
#define IDS_CM_INVALID_COMPOSE_DEPLOYMENT_OPERATION                     IDS_CM( 57 ) // Invalid operation on compose application. Use compose deployment operation.
#define IDS_CM_Compose_App_Name_Invalid                                 IDS_CM( 58 ) // Compose application name {0} should not be hierarchical.
#define IDS_CM_Compose_Deployment_Instance_Read_Failed                  IDS_CM( 59 ) // Persisted deployment data read failed for compose deployment {0}, error {1}
#define IDS_CM_Application_Type_Filter_Dup_Defined                      IDS_CM( 60 ) // At most one of ApplicationTypeName or ApplicationTypeDefinitionKindFilter can be specified.
#define IDS_CM_Invalid_Application_Definition_Kind_Filter               IDS_CM( 61 ) // The application definition kind filter is invalid.
#define IDS_CM_Invalid_Application_Type_Definition_Kind_Filter          IDS_CM( 62 ) // The application type definition kind filter is invalid.
#define IDS_CM_Compose_Deployment_Upgrade_In_Progress                   IDS_CM( 63 ) // Compose deployment {0} is being upgraded. Use the roll back api to interrupt the in progress upgrade before initiating a new upgrade.
#define IDS_CM_Deployment_Upgrading_To_Version                          IDS_CM( 64 ) // Deployment upgrading from version: {0} to version: {1}.
#define IDS_CM_Deployment_Upgraded_To_Version                           IDS_CM( 65 ) // Deployment upgraded to version: {0}.
#define IDS_CM_Deployment_Rollingback_To_Version                        IDS_CM( 66 ) // Deployment rolling back from version: {0} to version: {1}.
#define IDS_CM_Deployment_Rolledback_To_Version                         IDS_CM( 67 ) // Deployment rolled back to version: {0}.
#define IDS_CM_Deployment_Provisioning                                  IDS_CM( 68 ) // Provisioning new deployment version: {0}.
#define IDS_CM_Deployment_Upgrade_In_Progress                           IDS_CM( 69 ) // Deployment upgrade is in progress. Please check the deployment upgrade status for detailed information.
#define IDS_CM_Deployment_Upgrade_Failed                                IDS_CM( 70 ) // Previous deployment upgrade failed. Please check the deployment upgrade status for detailed information.
#define IDS_CM_MissingQueryParameter                                    IDS_CM( 71 ) // A required query parameter {0} is missing.
#define IDS_CM_Provision_Missing_Required_Parameter                     IDS_CM( 72 ) // Missing required parameter: please specify either the build path or the download path for the application package.
#define IDS_CM_Provision_Invalid_DownloadPath                           IDS_CM( 73 ) // The specified download path '{0}' is invalid. Please specify a valid URI with scheme HTTP/HTTPS that points to an '.sfpkg' file representing the application package.
#define IDS_CM_Provision_BuildPathAndDownloadPathAreExclusive           IDS_CM( 74 ) // Both the application package build path in image store '{0}' and the download path ApplicationPackageDownloadUri '{1}' are specified. Only one should be specified.
#define IDS_CM_ImageBuilder_Downloading_SfAppPkg                        IDS_CM( 75 ) // Running Image Builder process to download the application package from {0}.
#define IDS_CM_Provision_Missing_AppTypeInfo                            IDS_CM( 76 ) // To provision from external store, please specify the application type name and version.
#define IDS_CM_Invalid_Provision_Kind                                   IDS_CM( 77 ) // The specified provision application type kind '{0}' is invalid. Make sure the provision application type object follows the correct format, and 'Kind' field is serialized first.
#define IDS_CM_Invalid_Provision_Object                                 IDS_CM( 78 ) // The specified provision application type has an invalid format. Check whether the specified fields are valid and the 'Kind' field is serialized first.
#define IDS_CM_Not_SF_Application_Type                                  IDS_CM( 79 ) // Application type is not registered from a Service Fabric application package.
#define IDS_CM_Mismatched_Application2                                  IDS_CM( 80 ) // Service '{0}' already exists under a different application: requested='{1}' existing='{2}'
#define IDS_CM_Goal_State_App_Upgrade                                   IDS_CM( 81 ) // Upgrade will restart with target version '{0}' after rollback completes (use the explicit rollback API to clear)
#define IDS_CM_Invalid_Application_Parameter_Format                     IDS_CM( 82 ) // Cannot have newline characters \\r or \\n in application parameters. Found newline in parameter '{0}'
#define IDS_CM_Application_Read_Failed                                  IDS_CM( 83 ) // Persisted application context read failed for {0}, error {1}.
#define IDS_CM_Container_Group_Creation_Failed                          IDS_CM( 84 ) // Application creation failed due to '{0}' : {1}.
#define IDS_CM_Application_Type_Read_Failed                             IDS_CM( 85 ) // Persisted application type context read failed for {0} {1}, error {2}.
#define IDS_CM_Store_Data_Single_Instance_Application_Read_Failed       IDS_CM( 86 ) // Persisted store data single instance application instance read failed for {0}, {1}, error {2}.
#define IDS_CM_Invalid_Deployment_Type_Filter                           IDS_CM( 87 ) // The deployment type filter is invalid.
#define IDS_CM_Invalid_Application_Definition_Kind_Operation            IDS_CM( 88 ) // Invalid operation on {0} defined application. Use corresponding operation.
#define IDS_CM_Volume_Not_Found                                         IDS_CM( 89 ) // Volume {0} does not exist.
#define IDS_CM_VolumeNameNotSpecified                                   IDS_CM( 90 ) // Volume name not specified in volume description
#define IDS_CM_VolumeKindInvalid                                        IDS_CM( 91 ) // Volume kind is invalid
#define IDS_CM_VolumeDiskVolumeNameTooLong                              IDS_CM( 92 ) // Volume name length should not exceed {0} characters
#define IDS_CM_AzureFileVolumeAccountNameNotSpecified                   IDS_CM( 93 ) // Account name not specified for Azure Files volume {0}
#define IDS_CM_AzureFileVolumeAccountKeyNotSpecified                    IDS_CM( 94 ) // Account key not specified for Azure Files volume {0}
#define IDS_CM_AzureFileVolumeShareNameNotSpecified                     IDS_CM( 95 ) // Share name not specified for Azure Files volume {0}
#define IDS_CM_VolumeDescriptionMissing                                 IDS_CM( 96 ) // Volume description missing
#define IDS_CM_UnsupportedPreviewFeature_Upgrade_Failed                 IDS_CM( 97 ) // Cannot upgrade cluster with unsupported preview features enabled : version {0}
#define IDS_CM_AzureFileVolumeAccountKeyNotDisplayed                    IDS_CM( 98 ) // AccountKeyNotDisplayed
#define IDS_CM_InvalidDnsName_PatitionedQueryFormatInCompliance         IDS_CM( 99 ) // Service DNS name {0} should not match partitioned query format.


#define IDS_HM( Index )                                     RESOURCE_ID( IDS_MANAGEMENT( 200 ), Index )
#define IDS_HM_Health_Evaluation_Unhealthy_Event                        IDS_HM( 1 ) // '{0}' reported {1} for property '{2}'.
#define IDS_HM_Health_Evaluation_Expired_Event                          IDS_HM( 2 ) // The {0} reported by '{1}' for property '{2}' is expired. The report was applied at {3} with TTL {4}.
#define IDS_HM_Health_Evaluation_Unhealthy_EventPerPolicy               IDS_HM( 3 ) // '{0}' reported {1} for property '{2}'. The evaluation treats Warning as Error.
#define IDS_HM_Health_Evaluation_Unhealthy_Replicas                     IDS_HM( 4 ) // {0}% ({1}/{2}) replicas are unhealthy. The evaluation tolerates {3}% unhealthy replicas per partition.
#define IDS_HM_Health_Evaluation_Unhealthy_Partitions                   IDS_HM( 5 ) // {0}% ({1}/{2}) partitions are unhealthy. The evaluation tolerates {3}% unhealthy partitions per service.
#define IDS_HM_Health_Evaluation_Unhealthy_Deployed_Service_Packages    IDS_HM( 6 ) // {0}% ({1}/{2}) deployed service packages are unhealthy.
#define IDS_HM_Health_Evaluation_Unhealthy_DeployedApplications         IDS_HM( 7 ) // {0}% ({1}/{2}) deployed applications are unhealthy. The evaluation tolerates {3}% unhealthy deployed applications.
#define IDS_HM_Health_Evaluation_Unhealthy_ServiceType_Services         IDS_HM( 8 ) // {0}% ({1}/{2}) services of service type '{3}' are unhealthy. The evaluation tolerates {4}% unhealthy services for the service type.
#define IDS_HM_Health_Evaluation_Unhealthy_Nodes                        IDS_HM( 9 ) // {0}% ({1}/{2}) nodes are unhealthy. The evaluation tolerates {3}% unhealthy nodes.
#define IDS_HM_Health_Evaluation_Unhealthy_Applications                 IDS_HM( 10 ) // {0}% ({1}/{2}) applications are unhealthy. The evaluation tolerates {3}% unhealthy applications.
#define IDS_HM_Health_Evaluation_Unhealthy_UD_Deployed_Applications     IDS_HM( 11 ) // {0}% ({1}/{2}) deployed applications in upgrade domain '{3}' are unhealthy. The evaluation tolerates {4}% unhealthy deployed applications per upgrade domain.
#define IDS_HM_Health_Evaluation_Unhealthy_UD_Nodes                     IDS_HM( 12 ) // {0}% ({1}/{2}) nodes in upgrade domain '{3}' are unhealthy. The evaluation tolerates {4}% unhealthy nodes per upgrade domain.
#define IDS_HM_Health_Evaluation_Unhealthy_System_Application           IDS_HM( 13 ) // System application is in {0}.
#define IDS_HM_Health_Report_Reserved_SourceId                          IDS_HM( 14 ) // SourceId is a reserved name:
#define IDS_HM_Health_Report_Invalid_Empty_Input                        IDS_HM( 15 ) // Input parameter must be a non-empty string:
#define IDS_HM_Health_Report_Invalid_SourceId                           IDS_HM( 16 ) // SourceId must be a non-empty string with less characters than max length of
#define IDS_HM_Health_Report_Invalid_Property                           IDS_HM( 17 ) // Property must be a non-empty string with less characters than max length of
#define IDS_HM_Health_Report_Invalid_Description                        IDS_HM( 18 ) // Description must have less characters than max length of
#define IDS_HM_Health_Report_Invalid_Health_State                       IDS_HM( 19 ) // Specified health state is invalid:
#define IDS_HM_Health_Evaluation_Unhealthy_Node                         IDS_HM( 20 ) // Node '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_Replica                      IDS_HM( 21 ) // Replica '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_Partition                    IDS_HM( 22 ) // Partition '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_Service                      IDS_HM( 23 ) // Service '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_Application                  IDS_HM( 24 ) // Application '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_DeployedApplication          IDS_HM( 25 ) // Deployed application on node '{0}' is in {1}.
#define IDS_HM_Health_Evaluation_Unhealthy_DeployedServicePackage       IDS_HM( 26 ) // Service package for manifest '{0}' and service package activation ID '{1}' is in {2}.
#define IDS_HM_Health_Stale_Report_SequenceNumber                       IDS_HM( 27 ) // There is a health event with same SourceId and Property with equal or higher sequence number. Health report versus existing sequence numbers:
#define IDS_HM_Health_Stale_Report_Instance                             IDS_HM( 28 ) // The entity instance is stale. Health report versus existing instance:
#define IDS_HM_Health_Stale_Report_Entity_Deleted                       IDS_HM( 29 ) // The entity with the same instance {0} has been deleted.
#define IDS_HM_Health_Evaluation_Unhealthy_DeltaNodesCheck              IDS_HM( 30 ) // Delta nodes health check failed. There are {0}% ({1}/{2}) unhealthy nodes. Baseline: {3}% ({4}/{5}). MaxPercentDeltaUnhealthyNodes: {6}%.
#define IDS_HM_Health_Evaluation_Unhealthy_UpgradeDomainDeltaNodesCheck IDS_HM( 31 ) // Delta nodes health check for upgrade domain '{0}' failed. There are {1}% ({2}/{3}) unhealthy nodes. Baseline: {4}% ({5}/{6}). MaxPercentUpgradeDomainDeltaUnhealthyNodes: {7}%.
#define IDS_HM_QueryJobQueueFull                                        IDS_HM( 32 ) // Drop incoming query because of too many queued health queries:
#define IDS_HM_ApplicationNotFound                                      IDS_HM( 33 ) // Application is not found in Health Store:
#define IDS_HM_ApplicationPolicyNotSet                                  IDS_HM( 34 ) // Application policy is not set for application
#define IDS_HM_ClusterPolicyNotSet                                      IDS_HM( 35 ) // Cluster policy is not set.
#define IDS_HM_ParentApplicationNotFound                                IDS_HM( 36 ) // Parent application is not found in Health Store for
#define IDS_HM_ParentDeployedApplicationNotFound                        IDS_HM( 37 ) // Parent deployed application is not found in Health Store for
#define IDS_HM_ParentPartitionNotFound                                  IDS_HM( 38 ) // Parent partition is not found in Health Store for
#define IDS_HM_ParentServiceNotFound                                    IDS_HM( 39 ) // Parent service is not found in Health Store for
#define IDS_HM_ServiceTypeNotSet                                        IDS_HM( 40 ) // Service Type name is not set in Health Store for
#define IDS_HM_ParentNodeNotFound                                       IDS_HM( 41 ) // Parent node is not found in Health Store for
#define IDS_HM_InvalidQueryName                                         IDS_HM( 42 ) // The query name is unknown:
#define IDS_HM_MissingQueryParameter                                    IDS_HM( 43 ) // A required query parameter is missing. The query name and the parameter are:
#define IDS_HM_InvalidQueryParameter                                    IDS_HM( 44 ) // The query parameter is not in a valid format. Parameter name and value are:
#define IDS_HM_EntityDeletedOrCleanedUp                                 IDS_HM( 45 ) // Entity deleted from Health Store.
#define IDS_HM_NoHierarchySystemReport                                  IDS_HM( 46 ) // At least one parent entity is not found in Health Store or has no System report.
#define IDS_HM_NoSystemReport                                           IDS_HM( 47 ) // Entity has no System report.
#define IDS_HM_DeleteEntityDueToParent                                  IDS_HM( 48 ) // The deployed entity is pending deletion from Health Store due to node state (down or restarted):
#define IDS_HM_EntityNotFound                                           IDS_HM( 49 ) // Entity not found in Health Store.
#define IDS_HM_MissingRequiredReport                                    IDS_HM( 50 ) // Entity doesn't have a report from the System authority component {0}. This is usually transient, caused by message delays or timing issues. If it persists, check the authority component regarding this entity.
#define IDS_HM_SystemApplicationEvaluationError                         IDS_HM( 51 ) // There was an error evaluating fabric:/System application health.
#define IDS_HM_Health_Evaluation_Unhealthy_ApplicationTypeApplications  IDS_HM( 52 ) // {0}% ({1}/{2}) applications of type '{3}' are unhealthy. The evaluation tolerates {4}% unhealthy applications for this application type.
#define IDS_HM_Invalid_ApplicationTypeHealthPolicyMapItem               IDS_HM( 53 ) // Invalid application health policy map item at position
#define IDS_HM_Health_Report_Reserved_Property                          IDS_HM( 54 ) // Property is a reserved name:
#define IDS_HM_DuplicateNodeFilters                                     IDS_HM( 55 ) // The specified node filter list contains multiple entries for the same key. Node filter information:
#define IDS_HM_DuplicateApplicationFilters                              IDS_HM( 56 ) // The specified application filter list contains multiple entries for the same key. Application filter information:
#define IDS_HM_DuplicateServiceFilters                                  IDS_HM( 57 ) // The specified service filter list contains multiple entries for the same key. Application and service filter information:
#define IDS_HM_DuplicatePartitionFilters                                IDS_HM( 58 ) // The specified partition filter list contains multiple entries for the same key. Application, service and partition filter information:
#define IDS_HM_DuplicateReplicaFilters                                  IDS_HM( 59 ) // The specified replica filter list contains multiple entries for the same key. Application, service, partition and replica filter information:
#define IDS_HM_DuplicateDeployedApplicationFilters                      IDS_HM( 60 ) // The specified deployed application filter list contains multiple entries for the same key. Application and deployed application filter information:
#define IDS_HM_DuplicateDeployedServicePackageFilters                   IDS_HM( 61 ) // The specified deployed service package filter list contains multiple entries for the same key. Application, deployed application and deployed service package filter information:
#define IDS_HM_Health_Report_Invalid_Sequence_Number                    IDS_HM( 62 ) // Sequence number {0} is invalid.
#define IDS_HM_IncludeSystemApplication_With_ExcludeStats               IDS_HM( 63 ) // Can't set IncludeSystemApplicationHealthStatistics because ExcludeHealthStatistics is true.
#define IDS_HM_EntityTooLarge_NoUnhealthyEvaluations                    IDS_HM( 64 ) // The entity's estimated size {0} is bigger than max allowed size {1}. Consider applying filters to the query to reduce the returned information.
#define IDS_HM_EntityTooLarge_WithUnhealthyEvaluations                  IDS_HM( 65 ) // The trimmed entity doesn't fit in the max allowed size of {0}. Minimum size needed with trimmed evaluations is {1}.
#define IDS_HM_TooManyHealthReports                                     IDS_HM( 66 ) // The entity has {0} reports, which is more than the tolerated number, {1}. Check the health reporting logic. A possible cause is that the health report source and property are not correctly set.


#define IDS_RM( Index )                                     RESOURCE_ID( IDS_MANAGEMENT( 400 ), Index )
#define IDS_RM_Invalid_State_Transition                     IDS_RM( 1 ) // The state transition is not allowed:
#define IDS_RM_Missing_Required_Field                       IDS_RM( 2 ) // A required field is missing:
#define IDS_RM_Invalid_Field_Modification                   IDS_RM( 3 ) // Changes to the field are not allowed:
#define IDS_RM_Invalid_Field_Value                          IDS_RM( 4 ) // Field value is not valid or is not allowed in the current state:
#define IDS_RM_Invalid_Operation                            IDS_RM( 5 ) // The requested operation is not allowed in the current state:
#define IDS_RM_Repair_Cancelled                             IDS_RM( 6 ) // The repair request was canceled.

#define IDS_QUERY( Index )                                  RESOURCE_ID( IDS_MANAGEMENT( 600 ), Index )
#define IDS_QUERY_Query_Not_Supported                       IDS_QUERY( 1 ) // Query '{0}' is not supported. This can happen when the cluster is running an older version and does not recognize the newer query, or does not support the query configuration.
#define IDS_QUERY_Query_Missing_Required_Argument           IDS_QUERY( 2 ) // Query '{0}' is missing required argument {1}.
#define IDS_QUERY_Query_Unknown_Argument                    IDS_QUERY( 3 ) // Query '{0}' can't process argument {1}.
#define IDS_QUERY_Query_Arg_Not_Found                       IDS_QUERY( 4 ) // Required query arg '{1}' not found.
#define IDS_QUERY_Query_Arg_Invalid_Character               IDS_QUERY( 5 ) // Query arg {1} : '{2}' contains an invalid character {3}.

#define IDS_HTTP_GATEWAY( Index )                           RESOURCE_ID( IDS_MANAGEMENT( 800 ), Index )
#define IDS_HTTP_GATEWAY_Deserialization_Error              IDS_HTTP_GATEWAY( 1 ) // The request body can't be deserialized. Make sure it contains a valid {0} object.
#define IDS_HTTP_GATEWAY_Missing_Required_Parameter         IDS_HTTP_GATEWAY( 2 ) // A required parameter is missing: {0}.
#define IDS_HTTP_GATEWAY_Invalid_PartitionId_Parameter      IDS_HTTP_GATEWAY( 3 ) // The specified partition ID is invalid: {0}. Please specify a valid GUID.
#define IDS_HTTP_GATEWAY_Invalid_ReplicaId_Parameter        IDS_HTTP_GATEWAY( 4 ) // The specified replica ID is invalid: {0}.
#define IDS_HTTP_GATEWAY_Invalid_Filter_Parameter           IDS_HTTP_GATEWAY( 5 ) // The specified filter value is invalid: {0}. Please specify a valid number.
#define IDS_HTTP_GATEWAY_Missing_Parameter                  IDS_HTTP_GATEWAY( 6 ) // The specified parameter is missing: {0}.
#define IDS_HTTP_GATEWAY_Multiple_Entries                   IDS_HTTP_GATEWAY( 7 ) // The result contains multiple entries for the same name: {0}.

#define IDS_FEDERATION( Index )                             RESOURCE_ID( 16000, Index )
#define IDS_FEDERATION_Neighborhood_Lost                    IDS_FEDERATION( 1 )  // Neighborhood loss detected:

#define IDS_FABRIC_NODE( Index )                            RESOURCE_ID( 17000, Index )
#define IDS_FABRIC_NODE_Certificate_Expiration              IDS_FABRIC_NODE( 1 )  // Certificate expiration:
#define IDS_FABRIC_NODE_Certificate_Revocation_List         IDS_FABRIC_NODE( 2 )  // Certificate revocation list:
#define IDS_FABRIC_NODE_SecurityApi                         IDS_FABRIC_NODE( 3 )  // Security API:

#define IDS_HOSTING( Index )                                RESOURCE_ID( 18000, Index )
#define IDS_HOSTING_Download_Failed                         IDS_HOSTING( 1 )  // There was an error during download. 
#define IDS_HOSTING_Activation_Failed                       IDS_HOSTING( 2 )  // There was an error during activation.
#define IDS_HOSTING_Application_Activated                   IDS_HOSTING( 3 )  // The application was activated successfully.
#define IDS_HOSTING_ServicePackage_Activated                IDS_HOSTING( 4 )  // The ServicePackage was activated successfully.
#define IDS_HOSTING_CodePackage_Activation_Failed           IDS_HOSTING( 5 )  // There was an error during CodePackage activation.
#define IDS_HOSTING_CodePackage_Activated                   IDS_HOSTING( 6 )  // The CodePackage was activated successfully.
#define IDS_HOSTING_ServiceType_Registered                  IDS_HOSTING( 7 )  // The ServiceType was registered successfully.
#define IDS_HOSTING_ServiceType_Registration_Timeout        IDS_HOSTING( 8 )  // The ServiceType was not registered within the configured timeout.
#define IDS_HOSTING_ServiceType_Unregistered                IDS_HOSTING( 9 )  // The ServiceType was unregistered on the node since the Runtime or ApplicationHost closed.
#define IDS_HOSTING_ServiceType_Disabled                    IDS_HOSTING( 10 ) // The ServiceType was disabled on the node.
#define IDS_HOSTING_Invalid_PackageSharingPolicy            IDS_HOSTING( 11 ) // The specified PackageSharingPolicy is invalid. PackageSharingPolicy must contain a valid PackageName or valid SharingScope
#define IDS_HOSTING_Predeployment_NotAllowed                IDS_HOSTING(12) // Service packages cannot be copied to the node because Image Cache is disabled.
#define IDS_HOSTING_RunAsPolicy_NotEnabled                  IDS_HOSTING(13) // Application Manifest contains Principals, but RunAsPolicy is not enabled in ClusterManifest.
#define IDS_HOSTING_ResourceACL_Failed                      IDS_HOSTING(14) // Failed to ACL folders or certificates required by application. Error:
#define IDS_HOSTING_AppManifestDownload_Failed              IDS_HOSTING(15) // Failed to download Application Manifest for ApplicationType, Version:
#define IDS_HOSTING_ServiceManifestNotFound                 IDS_HOSTING(16) // Failed to find Service Manifest with name:
#define IDS_HOSTING_FabricUpgrade_Validation_Failed         IDS_HOSTING(17) // Fabric upgrade validation failed on the node.
#define IDS_HOSTING_FabricUpgrade_Failed         	        IDS_HOSTING(18) // Fabric upgrade failed on the node.
#define IDS_HOSTING_FabricDeployed_Output_Read_Failed       IDS_HOSTING(19) // There was an error reading the output of Fabric Deployer file.
#define IDS_HOSTING_FabricInstallerService_Setup_Failed     IDS_HOSTING(20) // There was an error setting up Fabric installer service
#define IDS_HOSTING_FabricInstallerService_Start_Failed     IDS_HOSTING(21) // There was an error starting Fabric installer service
#define IDS_HOSTING_ApplicationPrincipals_Setup_Failed      IDS_HOSTING(22) // Failed to setup ApplicationPrincipals. Error:
#define IDS_HOSTING_AppDownloadChecksum                     IDS_HOSTING(23) // The store checksum for package does not match the expected checksum.
#define IDS_HOSTING_ServiceHostActivationFailed             IDS_HOSTING(24) // Service host failed to activate. Error:
#define IDS_HOSTING_ServiceHostTerminated                   IDS_HOSTING(25) // The service host terminated with exit code:
#define IDS_HOSTING_ContainerImageDownloadFailed            IDS_HOSTING(26) // Failed to download container image
#define IDS_HOSTING_ContainerFeatureNotEnabled              IDS_HOSTING(27) // Container deployment is not supported on this node.

#define IDS_HOSTING_ContainerFailedToStart                  IDS_HOSTING(28) // Container failed to start for image:
#define IDS_HOSTING_ContainerCreationFailed                 IDS_HOSTING(29) // Container could not be created for image:
#define IDS_HOSTING_ContainerDeploymentNotSupported         IDS_HOSTING(30) // Container deployment is not supported on the node.
#define IDS_HOSTING_ContainerInvalidMountSource             IDS_HOSTING(31) // The source location specified for container mount is invalid. Source:
#define IDS_HOSTING_ActivationPathTooLong                   IDS_HOSTING(32) // Activation path for command line '{0}' not found. This can happen if the code package entry point path is too long.
#define IDS_HOSTING_ContainerVolumeCreationFailed           IDS_HOSTING(33) //  Failed to create volumes:
#define IDS_HOSTING_ContainerFailedToCreateDnsChain         IDS_HOSTING(34) //  Failed to create DNS chain for the container.

#define IDS_HOSTING_AvailableResourceCapacity_Mismatch      IDS_HOSTING(35)  // The node capacity defined in the cluster manifest is larger than the real node capacities for 
#define IDS_HOSTING_AvailableResourceCapacity_NotDefined    IDS_HOSTING(36)  // Auto-detection of resources is turned off in cluster manifest and node capacities are not defined for  
#define IDS_HOSTING_ContainerFailed_AddOutboundConnectivity               IDS_HOSTING(37)  // Failed to add outbound connectivity to container.
#define IDS_HOSTING_ContainerFailed_SetupUpdateRouteExecCommand           IDS_HOSTING(38)  // Failed to set up exec command to update route for container.
#define IDS_HOSTING_ContainerFailed_ResponseSetupUpdateRouteExecCommand   IDS_HOSTING(39)  // Failed to get response for set up exec command to update route for container.
#define IDS_HOSTING_ContainerFailed_ExecuteUpdateRouteCommand             IDS_HOSTING(40)  // Failed to execute command to update route for container.
#define IDS_HOSTING_EndpointBindingSetupFailed             IDS_HOSTING(41)  // Failed to configure endpoint with certificate {0}. Error {1}.
#define IDS_HOSTING_EndpointFirewallSetupFailed            IDS_HOSTING(42)  // Failed to configure firewall policy for endpoint. Error {0}.
#define IDS_HOSTING_EndpointHttpPortACLFailed              IDS_HOSTING(43)  // Failed to configure http port security for endpoint. Error {0}.
#define IDS_HOSTING_CertACLFailed                          IDS_HOSTING(44)  // Failed to configure certificate permissions. Error {0}.
#define IDS_HOSTING_FolderACLFailed                        IDS_HOSTING(45)  // Failed to configure application folder permissions. Error {0}.
#define IDS_HOSTING_DecryptDriverOptionFailed              IDS_HOSTING(46)  // Failed to decrypt driver option value for Name {0}. Error {1}.
#define IDS_HOSTING_DllLoadFailed                          IDS_HOSTING(47)  // Failed to load Dll {0}.
#define IDS_HOSTING_RtlGetVersionFailed                    IDS_HOSTING(48)  // Failed to execute RtlGetVersion to determine OS Build Number. Status {0}.
#define IDS_HOSTING_CertificateBindingAlreadyExists        IDS_HOSTING(49)  // There is already a certificate with thumbprint {0} bound to port {1}. New certificate thumbprint specified: {2}.
#define IDS_HOSTING_SecretStoreReferenceDecryptionFailed        IDS_HOSTING(50)  // Failed to retrieve SecreteStore Values for section:{0}, Ref:{1}.
#define IDS_HOSTING_FailedToLoadConfigSettingsFile         IDS_HOSTING(51)  // Couldn't find file Settings.xml {0}.
#define IDS_HOSTING_SecretStoreIsUnavailable               IDS_HOSTING(52)  // SecretService is not enabled. Please make sure you have IsEnabled set to true in ClusterManifest for section CentralSecretService.
#define IDS_HOSTING_DownloadFromStoreFailed                IDS_HOSTING(53)  // Failed to download '{0}' from ImageStore. Error '{1}'. Check if the file or folder is present in ImageStore by using 'Get-ServiceFabricImageStoreContent -RemoteRelativePath {2}'.
#define IDS_HOSTING_ExtractArchiveFailed                   IDS_HOSTING(54)  // Failed extracting '{0}' to '{1}' with Error: '{2}' while trying to {3}.

#define IDS_PLB( Index )                                    RESOURCE_ID( 19000, Index )
#define IDS_PLB_Node_Capacity_Violation                     IDS_PLB( 1 )  // The Cluster Resource Manager has detected a capacity violation for:
#define IDS_PLB_Node_Capacity_OK                            IDS_PLB( 2 )  // The Cluster Resource Manager has detected no capacity violations for:
#define IDS_PLB_Unplaced_Replica_Violation                  IDS_PLB( 4 )  // The Cluster Resource Manager was unable to find a placement for one or more of the Service's Replicas:
#define IDS_PLB_Replica_Constraint_Violation                IDS_PLB( 5 )  // The Cluster Resource Manager has detected a Constraint Violation for this Replica:
#define IDS_PLB_Replica_Soft_Constraint_Violation           IDS_PLB( 6 )  // The Cluster Resource Manager has detected a Soft Constraint Violation for this Replica:
#define IDS_PLB_Upgrade_Primary_Swap_Violation              IDS_PLB( 7 )  // The Cluster Resource Manager was unable to issue Upgrade Primary Swaps for this Partition:
#define IDS_PLB_Balancing_Unsuccessful                      IDS_PLB( 8 )  // The Cluster Resource Manager was unable to balance services among these Metrics:
#define IDS_PLB_ConstraintFix_Unsuccessful                  IDS_PLB( 9 )  // The Cluster Resource Manager was unable to fix the constraint violation for one or more of the Service's Replicas:
#define IDS_PLB_Correlation_Error                           IDS_PLB( 10 ) // The Cluster Resource Manager detected affinity chain for service:
#define IDS_PLB_Service_Description_OK                      IDS_PLB( 11 ) // The Cluster Resource Manager successfully updated the service:
#define IDS_PLB_Movements_Dropped                           IDS_PLB( 12 ) // Movements that the Cluster Resource Manager issued were dropped before execution:
#define IDS_PLB_Movements_Dropped_By_FM_Description         IDS_PLB( 13 ) // Check for relevant unhealthy replicas and paused, deactivating, or unhealthy nodes in the cluster and consider restarting them as a possible mitigation.

#define IDS_RE( Index )                                     RESOURCE_ID( 20000, Index )
#define IDS_RE_QueueFull                                    IDS_RE( 1 ) // Replicator queue full:
#define IDS_RE_QueueOk                                      IDS_RE( 2 ) // Replicator queue OK:
#define IDS_RE_RemoteReplicatorConnectionStatusOk           IDS_RE( 3 ) // Remote replicator connection status OK:
#define IDS_RE_RemoteReplicatorConnectionStatusFailed       IDS_RE( 4 ) // Remote replicator connection status FAILED:

#define IDS_FSS( Index )                                    RESOURCE_ID( 21000, Index )
#define IDS_FSS_Invalid_SecurityPrincipalAccountType        IDS_FSS( 1 ) // The security principal account type '{0}' is not recognized as a valid value.
#define IDS_FSS_Staging_SMBCopy_NetworkFailure              IDS_FSS( 2 ) // Unable to copy file to the image store service primary's staging location due to network error '{0}'. Please make sure SMB port 445 is enabled in the cluster's Network Security Group(NSG).
#define IDS_FSS_Staging_SMBCopyFailure                      IDS_FSS( 3 ) // Unable to copy file to the image store service primary's staging location due to error '{0}'. Please retry.
#define IDS_FSS_CloseTo_OutOfDiskSpace                      IDS_FSS( 4 ) // Disk {0} on node {1} has {2}% free space ({3} free bytes / {4} total bytes), which is less than desired limit of {5} bytes. Image store size is {6} bytes ({7}% of total disk space). Potential ways to reduce space are removing unused application packages and application types.
#define IDS_FSS_DiskSpaceOk                                 IDS_FSS( 5 ) // Enough disk space available on node:{0} volume:{1}
#define IDS_FSS_UnexpectedMergeSize                         IDS_FSS( 6 ) // Upload failed due to an internal error. Image store file {0} has {1} bytes, which doesn't match expected value of {2} bytes. Please retry.
#define IDS_FSS_UnexpectedStagingChunkSize                  IDS_FSS( 7 ) // Upload failed due to an internal error. File chunk received has {0} bytes, which doesn't match expected value of {1} bytes. Please retry.

#define IDS_FSS_CLIENT( Index )                             RESOURCE_ID( IDS_FSS( 500 ), Index )
#define IDS_FSS_CLIENT_SendChunkFailure                     IDS_FSS_CLIENT( 1 ) // Unable to send file chunks to the image store. Please retry after network congestion is reduced.
#define IDS_FSS_CLIENT_UnableToUploadFile                   IDS_FSS_CLIENT( 2 ) // Unable to upload a file to the image store relative path '{0}'. Please retry.
#define IDS_FSS_CLIENT_OperationCanceled                    IDS_FSS_CLIENT( 3 ) // The operation is canceled by the cluster. Please retry.
#define IDS_FSS_CLIENT_ConnectionConfirmWaitExpired         IDS_FSS_CLIENT( 4 ) // Unable to establish connection with image store service. Please retry.
#define IDS_FSS_CLIENT_OperationFailed                      IDS_FSS_CLIENT( 5 ) // The operation failed due to internal error in uploading a file to the image store path '{0}'. Please retry.
#define IDS_FSS_CLIENT_FSSNtlmAuthenticationDisabled        IDS_FSS_CLIENT( 6 ) // Unable to upload application package to the cluster due to the usage of obsolete upload protocol. Please use latest SDK client to upload the package.

#define IDS_FABRICAPPLICATIONGATEWAY( Index )                            RESOURCE_ID( IDS_EXE( 12000 ), Index )
#define IDS_FABRICAPPLICATIONGATEWAY_STARTING                            IDS_FABRICAPPLICATIONGATEWAY( 1 ) // Starting Fabric Application Gateway.
#define IDS_FABRICAPPLICATIONGATEWAY_STARTED                             IDS_FABRICAPPLICATIONGATEWAY( 2 ) // Fabric Application Gateway started.
#define IDS_FABRICAPPLICATIONGATEWAY_EXIT_PROMPT                         IDS_FABRICAPPLICATIONGATEWAY( 3 ) // Press [Enter] to exit.
#define IDS_FABRICAPPLICATIONGATEWAY_CLOSING                             IDS_FABRICAPPLICATIONGATEWAY( 4 ) // Closing Fabric Application Gateway.
#define IDS_FABRICAPPLICATIONGATEWAY_CLOSED                              IDS_FABRICAPPLICATIONGATEWAY( 5 ) // Fabric Application Gateway closed.
#define IDS_FABRICAPPLICATIONGATEWAY_CTRL_C                              IDS_FABRICAPPLICATIONGATEWAY( 6 ) // [Ctrl+C] caught.
#define IDS_FABRICAPPLICATIONGATEWAY_CTRL_BRK                            IDS_FABRICAPPLICATIONGATEWAY( 7 ) // [Ctrl+Break] caught.
#define IDS_FABRICAPPLICATIONGATEWAY_WARN_SHUTDOWN                       IDS_FABRICAPPLICATIONGATEWAY( 8 ) // Shutting down while open is still pending.

#define IDS_TESTABILITY( Index )                            RESOURCE_ID( 22000, Index )
#define IDS_TESTABILITY_UnreliableTransportWarning          IDS_TESTABILITY(1) // Unreliable transport behavior is active beyond threshold time:

#define IDS_FAS( Index )                                    RESOURCE_ID( 24000, Index )
#define IDS_FAS_DataLossModeInvalid                         IDS_FAS( 1 ) // Invalid is a reserved DataLossMode and should not be used.
#define IDS_FAS_DataLossModeUnknown                         IDS_FAS( 2 ) // Unknown DataLossMode.
#define IDS_FAS_QuorumLossModeInvalid                       IDS_FAS( 3 ) // Invalid is a reserved QuorumLossMode and should not be used.
#define IDS_FAS_QuorumLossModeUnknown                       IDS_FAS( 4 ) // Unknown QuorumLossMode.
#define IDS_FAS_RestartPartitionModeInvalid                 IDS_FAS( 5 ) // Invalid is a reserved RestartPartitionMode and should not be used.
#define IDS_FAS_RestartPartitionModeUnknown                 IDS_FAS( 6 ) // Unknown RestartPartitionMode.
#define IDS_FAS_ChaosStatusUnknown                          IDS_FAS( 7 ) // Unknown ChaosStatus.
#define IDS_FAS_GetChaosReportArgumentInvalid               IDS_FAS( 9 ) // Invalid argument combination in GetChaosReport; use either ChaosReportFilter or ContinuationToken, but not both.
#define IDS_FAS_ChaosTimeToRunTooBig                        IDS_FAS( 10 ) // Too big a timeToRun has been passed into the API; the maximum timeToRun must not exceed 4294967295 seconds.
#define IDS_FAS_ChaosContextNullKeyOrValue                  IDS_FAS( 11 ) // Context dictionary in ChaosParameters cannot have null key or null value
#define IDS_FAS_ChaosNodeTypeInclusionListTooLong           IDS_FAS( 12 ) // NodeTypeInclusionList cannot have more than 100 NodeTypes.
#define IDS_FAS_ChaosApplicationInclusionListTooLong        IDS_FAS( 13 ) // ApplicationInclusionList cannot have more than 1000 applications.
#define IDS_FAS_ChaosTargetFilterSpecificationNotFound      IDS_FAS( 14 ) // NodeTypeInclusionList and ApplicationInclusionList in ChaosTargetFilter cannot both be empty.
#define IDS_FAS_ChaosScheduleStatusUnknown                  IDS_FAS( 15 ) // Unknown ChaosScheduleStatus.
#define IDS_FAS_GetChaosEventsArgumentInvalid               IDS_FAS( 16 ) // Invalid argument combination in GetChaosEvents; use either ChaosEventsFilter or ContinuationToken, but not both.

//
// IDS_ERROR_MESSAGE is used by HTTP gateway to return friendly error message strings
//

#define IDS_ERROR_MESSAGE( Index )                                                                    RESOURCE_ID( 25000, Index )
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_KEY                                      IDS_ERROR_MESSAGE( 1 )  // Invalid partition key/ID
#define IDS_ERROR_MESSAGE_FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED                IDS_ERROR_MESSAGE( 2 )  // Certificate for UserRole FabricClient is not configured.
#define IDS_ERROR_MESSAGE_FABRIC_E_NAME_ALREADY_EXISTS                                        IDS_ERROR_MESSAGE( 3 )  // Name already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_ALREADY_EXISTS                                     IDS_ERROR_MESSAGE( 4 )  // Service already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS                               IDS_ERROR_MESSAGE( 5 )  // Service group already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS                            IDS_ERROR_MESSAGE( 6 )  // Application type and version already exists
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_ALREADY_EXISTS                                 IDS_ERROR_MESSAGE( 7 )  // Application already exists
#define IDS_ERROR_MESSAGE_FABRIC_E_NAME_DOES_NOT_EXIST                                        IDS_ERROR_MESSAGE( 8 )  // Name does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_PROPERTY_DOES_NOT_EXIST                                    IDS_ERROR_MESSAGE( 9 )  // Property does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_DOES_NOT_EXIST                                     IDS_ERROR_MESSAGE( 10 )  // Service does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST                               IDS_ERROR_MESSAGE( 11 )  // Service group does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS                     IDS_ERROR_MESSAGE( 12 )  // Cannot provision multiple versions of the same application type simultaneously.
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_NOT_FOUND                                 IDS_ERROR_MESSAGE( 13 )  // Application type and version not found
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_IN_USE                                    IDS_ERROR_MESSAGE( 14 )  // Application type and version is still in use
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_NOT_FOUND                                      IDS_ERROR_MESSAGE( 15 )  // Application not found
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_NOT_FOUND                                     IDS_ERROR_MESSAGE( 16 )  // Service Type not found
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND                            IDS_ERROR_MESSAGE( 17 )  // Service Type template not found
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_MANIFEST_NOT_FOUND                                 IDS_ERROR_MESSAGE( 18 )  // Service Manifest not found
#define IDS_ERROR_MESSAGE_FABRIC_E_NOT_PRIMARY                                                IDS_ERROR_MESSAGE( 19 )  // The operation failed because this node is not the primary replica.  Consider re-resolving the primary and retrying the operation there.
#define IDS_ERROR_MESSAGE_FABRIC_E_NO_WRITE_QUORUM                                            IDS_ERROR_MESSAGE( 20 )  // The operation failed because a quorum of nodes are not available for this replica set.  Consider retrying once more nodes are up.
#define IDS_ERROR_MESSAGE_FABRIC_E_RECONFIGURATION_PENDING                                    IDS_ERROR_MESSAGE( 21 )  // The operation failed because this replica set is currently reconfiguring.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPLICATION_QUEUE_FULL                                     IDS_ERROR_MESSAGE( 22 )  // Replication queue is full.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPLICATION_OPERATION_TOO_LARGE                            IDS_ERROR_MESSAGE( 23 )  // The replication operation is larger than the configured limit - MaxReplicationMessageSize
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ATOMIC_GROUP                                       IDS_ERROR_MESSAGE( 24 )  // The specified atomic group has not been created or no longer exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_OFFLINE                                            IDS_ERROR_MESSAGE( 25 )  // The operation failed because the service is currently offline.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_METADATA_MISMATCH                                  IDS_ERROR_MESSAGE( 26 )  // The operation failed because the service metadata does not match.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED                       IDS_ERROR_MESSAGE( 27 )  // The operation failed because creating the service would form an affinity chain which is not supported.
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS                            IDS_ERROR_MESSAGE( 28 )  // Application is currently being upgraded
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS                             IDS_ERROR_MESSAGE( 29 )  // Application is currently being updated
#define IDS_ERROR_MESSAGE_FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED                           IDS_ERROR_MESSAGE( 30 )  // Specified upgrade domain has already completed.
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_NOT_FOUND                                   IDS_ERROR_MESSAGE( 31 )  // Fabric version has not been registered
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_IN_USE                                      IDS_ERROR_MESSAGE( 32 )  // Fabric is currently in this version or being upgraded to this version
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS                              IDS_ERROR_MESSAGE( 33 )  // Version has already been registered
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION                           IDS_ERROR_MESSAGE( 34 )  // Fabric is already in this version
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_NOT_UPGRADING                                       IDS_ERROR_MESSAGE( 35 )  // There is no pending Fabric upgrade
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS                                 IDS_ERROR_MESSAGE( 36 )  // Fabric is being upgraded
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR                            IDS_ERROR_MESSAGE( 37 )  // Fabric upgrade request failed validation
#define IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_MAX_REPORTS_REACHED                                 IDS_ERROR_MESSAGE( 38 )  // Max number of health reports reached, try again
#define IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_STALE_REPORT                                        IDS_ERROR_MESSAGE( 39 )  // Health report is stale
#define IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_ENTITY_NOT_FOUND                                    IDS_ERROR_MESSAGE( 40 )  // Entity not found in Health Store
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_NOT_UPGRADING                                  IDS_ERROR_MESSAGE( 41 )  // Application is currently NOT being upgraded
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION                      IDS_ERROR_MESSAGE( 42 )  // Application is already in the target version
#define IDS_ERROR_MESSAGE_FABRIC_E_NODE_NOT_FOUND                                             IDS_ERROR_MESSAGE( 43 )  // Node not found.
#define IDS_ERROR_MESSAGE_FABRIC_E_NODE_IS_UP                                                 IDS_ERROR_MESSAGE( 44 )  // Node is up.
#define IDS_ERROR_MESSAGE_FABRIC_E_WRITE_CONFLICT                                             IDS_ERROR_MESSAGE( 45 )  // Write conflict.
#define IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR                       IDS_ERROR_MESSAGE( 46 )  // Invalid application upgrade request
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_MISMATCH                                      IDS_ERROR_MESSAGE( 47 )  // Service Type mismatches description type (i.e. stateless/stateful/persisted)
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED                            IDS_ERROR_MESSAGE( 48 )  // Service Type is already registered.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_NOT_REGISTERED                                IDS_ERROR_MESSAGE( 49 )  // Service Type is not registered.
#define IDS_ERROR_MESSAGE_FABRIC_E_CODE_PACKAGE_NOT_FOUND                                     IDS_ERROR_MESSAGE( 50 )  // The code package was not found
#define IDS_ERROR_MESSAGE_FABRIC_E_DATA_PACKAGE_NOT_FOUND                                     IDS_ERROR_MESSAGE( 51 )  // The data package was not found
#define IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND                            IDS_ERROR_MESSAGE( 52 )  // The config package was not found
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND                        IDS_ERROR_MESSAGE( 53 )  // The Endpoint resource was not found
#define IDS_ERROR_MESSAGE_FABRIC_E_NAME_NOT_EMPTY                                             IDS_ERROR_MESSAGE( 54 )  // Name not empty.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CONFIGURATION                                      IDS_ERROR_MESSAGE( 55 )  // A configuration parameter has been set to an invalid value. Please check the logs for details.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ADDRESS                                            IDS_ERROR_MESSAGE( 56 )  // The supplied address was invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_NAME_URI                                           IDS_ERROR_MESSAGE( 57 )  // Invalid name URI.
#define IDS_ERROR_MESSAGE_FABRIC_E_VALUE_TOO_LARGE                                            IDS_ERROR_MESSAGE( 58 )  // Invalid argument
#define IDS_ERROR_MESSAGE_FABRIC_E_VALUE_EMPTY                                                IDS_ERROR_MESSAGE( 59 )  // The property value is empty.
#define IDS_ERROR_MESSAGE_FABRIC_E_PROPERTY_CHECK_FAILED                                      IDS_ERROR_MESSAGE( 60 )  // The property check failed.
#define IDS_ERROR_MESSAGE_FABRIC_E_NOT_READY                                                  IDS_ERROR_MESSAGE( 61 )  // The requested data is not yet in cache.  This same operation is likely to succeed if retried later.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CREDENTIAL_TYPE                                    IDS_ERROR_MESSAGE( 62 )  // The security credential type specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_FIND_TYPE                                     IDS_ERROR_MESSAGE( 63 )  // The X509 find type specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE_LOCATION                                IDS_ERROR_MESSAGE( 64 )  // The X509 store location specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE_NAME                                    IDS_ERROR_MESSAGE( 65 )  // The X509 store name specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_THUMBPRINT                                    IDS_ERROR_MESSAGE( 66 )  // The X509 thumbprint specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PROTECTION_LEVEL                                   IDS_ERROR_MESSAGE( 67 )  // The protection level specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE                                         IDS_ERROR_MESSAGE( 68 )  // The X509 store specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_SUBJECT_NAME                                       IDS_ERROR_MESSAGE( 69 )  // The X509 subject name specified is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST                           IDS_ERROR_MESSAGE( 70 )  // The specified list of allowed common names is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CREDENTIALS                                        IDS_ERROR_MESSAGE( 71 )  // The specified credentials are invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_DECRYPTION_FAILED                                          IDS_ERROR_MESSAGE( 72 )  // Failed to decrypt the value.
#define IDS_ERROR_MESSAGE_FABRIC_E_ENCRYPTION_FAILED                                          IDS_ERROR_MESSAGE( 73 )  // Failed to encrypt the value.
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TOO_BUSY                                           IDS_ERROR_MESSAGE( 74 )  // The service cannot process the request because it is too busy. Please retry.
#define IDS_ERROR_MESSAGE_FABRIC_E_COMMUNICATION_ERROR                                        IDS_ERROR_MESSAGE( 75 )  // A communication error caused the operation to fail.
#define IDS_ERROR_MESSAGE_FABRIC_E_GATEWAY_NOT_REACHABLE                                      IDS_ERROR_MESSAGE( 76 )  // A communication error caused the operation to fail.
#define IDS_ERROR_MESSAGE_FABRIC_E_OBJECT_CLOSED                                              IDS_ERROR_MESSAGE( 77 )  // The object is closed.
#define IDS_ERROR_MESSAGE_FABRIC_E_OPERATION_NOT_COMPLETE                                     IDS_ERROR_MESSAGE( 78 )  // The operation has not completed.
#define IDS_ERROR_MESSAGE_FABRIC_E_ENUMERATION_COMPLETED                                      IDS_ERROR_MESSAGE( 79 )  // Enumeration has completed.
#define IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND                            IDS_ERROR_MESSAGE( 80 )  // A configuration section is missing.
#define IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND                          IDS_ERROR_MESSAGE( 81 )  // A configuration parameter is missing.
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR                              IDS_ERROR_MESSAGE( 82 )  // There is a content validation error in the manifest file(s)
#define IDS_ERROR_MESSAGE_FABRIC_E_PARTITION_NOT_FOUND                                        IDS_ERROR_MESSAGE( 83 )  // The operation failed because the requested partition does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPLICA_DOES_NOT_EXIST                                     IDS_ERROR_MESSAGE( 84 )  // The specified replica does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_PROCESS_DEACTIVATED                                        IDS_ERROR_MESSAGE( 85 )  // The process has been deactivated.
#define IDS_ERROR_MESSAGE_FABRIC_E_PROCESS_ABORTED                                            IDS_ERROR_MESSAGE( 86 )  // The process has aborted.
#define IDS_ERROR_MESSAGE_FABRIC_E_KEY_TOO_LARGE                                              IDS_ERROR_MESSAGE( 87 )  // The key is too large.
#define IDS_ERROR_MESSAGE_FABRIC_E_KEY_NOT_FOUND                                              IDS_ERROR_MESSAGE( 88 )  // The given key was not present.
#define IDS_ERROR_MESSAGE_FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED                               IDS_ERROR_MESSAGE( 89 )  // Sequence number check failed
#define IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_NOT_ACTIVE                                     IDS_ERROR_MESSAGE( 90 )  // Transaction has already committed or rolled back
#define IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_TOO_LARGE                                      IDS_ERROR_MESSAGE( 91 )  // Transaction data exceeds the configured replication message size limit - ReplicatorSettings.MaxReplicationMessageSize
#define IDS_ERROR_MESSAGE_FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED                     IDS_ERROR_MESSAGE( 92 )  // Multithreaded usage of transactions is not allowed.
#define IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_ABORTED                                        IDS_ERROR_MESSAGE( 93 )  // Transaction cannot be used after encountering an error. Retries must occur on a new transaction.
#define IDS_ERROR_MESSAGE_E_INVALIDARG                                                        IDS_ERROR_MESSAGE( 94 )  // Invalid argument
#define IDS_ERROR_MESSAGE_E_POINTER                                                           IDS_ERROR_MESSAGE( 95 )  // Invalid argument
#define IDS_ERROR_MESSAGE_E_ABORT                                                             IDS_ERROR_MESSAGE( 96 )  // Operation canceled.
#define IDS_ERROR_MESSAGE_E_ACCESSDENIED                                                      IDS_ERROR_MESSAGE( 97 )  // Access denied.
#define IDS_ERROR_MESSAGE_E_NOINTERFACE                                                       IDS_ERROR_MESSAGE( 98 )  // Invalid cast.
#define IDS_ERROR_MESSAGE_E_NOTIMPL                                                           IDS_ERROR_MESSAGE( 99 )  // The method or operation is not implemented.
#define IDS_ERROR_MESSAGE_E_OUTOFMEMORY                                                       IDS_ERROR_MESSAGE( 100 )  // Insufficient memory to continue the execution of the program.
#define IDS_ERROR_MESSAGE_E_NOT_FOUND                                                         IDS_ERROR_MESSAGE( 101 )  // Element not found
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_OPERATION                                          IDS_ERROR_MESSAGE( 102 )  // The operation is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_TIMEOUT                                                    IDS_ERROR_MESSAGE( 103 )  // Operation timed out.
#define IDS_ERROR_MESSAGE_FABRIC_E_DIRECTORY_NOT_FOUND                                        IDS_ERROR_MESSAGE( 104 )  // Directory not found
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_DIRECTORY                                          IDS_ERROR_MESSAGE( 105 )  // Invalid Directory
#define IDS_ERROR_MESSAGE_FABRIC_E_PATH_TOO_LONG                                              IDS_ERROR_MESSAGE( 106 )  // Path is too long
#define IDS_ERROR_MESSAGE_FABRIC_E_FILE_NOT_FOUND                                             IDS_ERROR_MESSAGE( 107 )  // File not found
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGESTORE_IOERROR                                         IDS_ERROR_MESSAGE( 108 )  // There was an IOException when using the Image Store.
#define IDS_ERROR_MESSAGE_FABRIC_E_ACQUIRE_FILE_LOCK_FAILED                                   IDS_ERROR_MESSAGE( 109 )  // There was an error acquiring lock on the file. This indicates that another process has acquired write lock on the file or the process does not have access to the file location.
#define IDS_ERROR_MESSAGE_FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND                         IDS_ERROR_MESSAGE( 110 )  // The Image Store object related to this operation appears corrupted. Please re-upload to the Image Store incoming folder before retrying the operation.
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR                              IDS_ERROR_MESSAGE( 111 )  // The Image Builder encountered an unexpected error.
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_TIMEOUT                                       IDS_ERROR_MESSAGE( 112 )  // Image Builder timed out. Please check that the Image Store is available and consider providing a larger timeout when processing large application packages.
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_ACCESS_DENIED                                 IDS_ERROR_MESSAGE( 113 )  // Please check that the Image Store is available and has correct access permissions for Microsoft Azure Service Fabric processes.
#define IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE                              IDS_ERROR_MESSAGE( 114 )  // The MSI file is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_SERVICE_TYPE                                       IDS_ERROR_MESSAGE( 115 )  // Invalid Service Type
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE                 IDS_ERROR_MESSAGE( 116 )  // Reliable session transport startup failure
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS                            IDS_ERROR_MESSAGE( 117 )  // Reliable session already exists
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT                            IDS_ERROR_MESSAGE( 118 )  // Reliable session cannot connect
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS                            IDS_ERROR_MESSAGE( 119 )  // Reliable session manager exists
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_REJECTED                                  IDS_ERROR_MESSAGE( 120 )  // Reliable session rejected
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_NOT_FOUND                                 IDS_ERROR_MESSAGE( 121 )  // Reliable session not found
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY                               IDS_ERROR_MESSAGE( 122 )  // Reliable session queue empty
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED                            IDS_ERROR_MESSAGE( 123 )  // Reliable session quota exceeded
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED                           IDS_ERROR_MESSAGE( 124 )  // Reliable session service faulted
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING                 IDS_ERROR_MESSAGE( 125 )  // Reliable session manager already listening
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND                         IDS_ERROR_MESSAGE( 126 )  // Reliable session manager not found
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING                     IDS_ERROR_MESSAGE( 127 )  // Reliable session manager not listening
#define IDS_ERROR_MESSAGE_FABRIC_E_REPAIR_TASK_ALREADY_EXISTS                                 IDS_ERROR_MESSAGE( 128 )  // The repair task already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPAIR_TASK_NOT_FOUND                                      IDS_ERROR_MESSAGE( 129 )  // The repair task could not be found.
#define IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION                  IDS_ERROR_MESSAGE( 130 )  // Reliable session invalid target partition provided
#define IDS_ERROR_MESSAGE_FABRIC_E_INSTANCE_ID_MISMATCH                                       IDS_ERROR_MESSAGE( 131 )  // The provided InstanceId did not match.
#define IDS_ERROR_MESSAGE_FABRIC_E_NODE_HAS_NOT_STOPPED_YET                                   IDS_ERROR_MESSAGE( 132 )  // Node has not stopped yet - a previous StopNode is still pending
#define IDS_ERROR_MESSAGE_FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY                              IDS_ERROR_MESSAGE( 133 )  // The cluster did not have enough resources to perform the requested operation.
#define IDS_ERROR_MESSAGE_FABRIC_E_CONSTRAINT_KEY_UNDEFINED                                   IDS_ERROR_MESSAGE( 134 )  // One or more placement constraints on the service are undefined on all nodes that are currently up.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PACKAGE_SHARING_POLICY                             IDS_ERROR_MESSAGE( 135 )  // PackageSharingPolicy must contain a valid PackageName or Scope
#define IDS_ERROR_MESSAGE_FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED                                  IDS_ERROR_MESSAGE( 136 )  // Service Manifest packages could not be deployed to node because Image Cache is disabled
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_BACKUP_SETTING                                     IDS_ERROR_MESSAGE( 137 )  // Invalid backup setting. E.g. incremental backup option is not set upfront
#define IDS_ERROR_MESSAGE_FABRIC_E_MISSING_FULL_BACKUP                                        IDS_ERROR_MESSAGE( 138 )  // Incremental backups can only be done after an initial full backup
#define IDS_ERROR_MESSAGE_FABRIC_E_BACKUP_IN_PROGRESS                                         IDS_ERROR_MESSAGE( 139 )  // A previously initiated backup is currently in progress
#define IDS_ERROR_MESSAGE_FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY                                 IDS_ERROR_MESSAGE( 140 )  // The backup directory is not empty
#define IDS_ERROR_MESSAGE_FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME                 IDS_ERROR_MESSAGE( 141 )  // Service notification filter has already been registered at the specified name.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_OPERATION                                  IDS_ERROR_MESSAGE( 142 )  // Restart can only be performed on persisted services. For volatile or stateless services use Remove.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_STATE                                      IDS_ERROR_MESSAGE( 143 )  // This operation cannot be performed in the current replica state.
#define IDS_ERROR_MESSAGE_FABRIC_E_LOADBALANCER_NOT_READY                                     IDS_ERROR_MESSAGE( 144 )  // Cluster Resource Manager is currently busy.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_OPERATION                                IDS_ERROR_MESSAGE( 145 )  // MovePrimary or MoveSecondary operation can be only be performed on stateful Service Type.
#define IDS_ERROR_MESSAGE_FABRIC_E_PRIMARY_ALREADY_EXISTS                                     IDS_ERROR_MESSAGE( 146 )  // Replica is already primary role.
#define IDS_ERROR_MESSAGE_FABRIC_E_SECONDARY_ALREADY_EXISTS                                   IDS_ERROR_MESSAGE( 147 )  // Replica is already secondary role.
#define IDS_ERROR_MESSAGE_FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION                  IDS_ERROR_MESSAGE( 148 )  // Replicas hosted in Fabric.exe or in processes not managed by Microsoft Azure Service Fabric cannot be force removed.
#define IDS_ERROR_MESSAGE_FABRIC_E_CONNECTION_DENIED                                          IDS_ERROR_MESSAGE( 149 )  // Not authorized to connect
#define IDS_ERROR_MESSAGE_FABRIC_E_SERVER_AUTHENTICATION_FAILED                               IDS_ERROR_MESSAGE( 150 )  // Failed to authenticate server identity
#define IDS_ERROR_MESSAGE_FABRIC_E_CANNOT_CONNECT                                             IDS_ERROR_MESSAGE( 151 )  // Error in Connection during ServiceCommunication
#define IDS_ERROR_MESSAGE_FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END                            IDS_ERROR_MESSAGE( 152 )  // Error in Connection during ServiceCommunication
#define IDS_ERROR_MESSAGE_FABRIC_E_MESSAGE_TOO_LARGE                                          IDS_ERROR_MESSAGE( 153 )  // Fabric Message is too large
#define IDS_ERROR_MESSAGE_FABRIC_E_CONSTRAINT_NOT_SATISFIED                                   IDS_ERROR_MESSAGE( 154 )  // The attempted move would have resulted in replica placements that violate one or more constraints. The move was not executed.
#define IDS_ERROR_MESSAGE_FABRIC_E_ENDPOINT_NOT_FOUND                                         IDS_ERROR_MESSAGE( 155 )  // Fabric Endpoint not found during connection
#define IDS_ERROR_MESSAGE_FABRIC_E_DELETE_BACKUP_FILE_FAILED                                  IDS_ERROR_MESSAGE( 156 )  // Failed to delete file/folder during a backup operation.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_TEST_COMMAND_STATE                                 IDS_ERROR_MESSAGE( 157 )  // This test command operation is not valid for the current state of the test command.
#define IDS_ERROR_MESSAGE_FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS                   IDS_ERROR_MESSAGE( 158 )  // This test command operation already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_CM_OPERATION_FAILED                                        IDS_ERROR_MESSAGE( 159 )  // Creation or deletion was terminated due to persistent failures.
#define IDS_ERROR_MESSAGE_FABRIC_E_CHAOS_ALREADY_RUNNING                                      IDS_ERROR_MESSAGE( 160 )  // An instance of Chaos is already running in the cluster.
#define IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_DATA_ROOT_NOT_FOUND                                 IDS_ERROR_MESSAGE( 161 )  // Fabric Data Root was not found on the machine.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_SELECTOR                                 IDS_ERROR_MESSAGE( 162 )  // The partition selector is invalid
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_SELECTOR                                   IDS_ERROR_MESSAGE( 163 )  // The replica selector is invalid
#define IDS_ERROR_MESSAGE_FABRIC_E_DNS_SERVICE_NOT_FOUND                                      IDS_ERROR_MESSAGE( 164 )  // DnsService is not enabled on the cluster.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_DNS_NAME                                           IDS_ERROR_MESSAGE( 165 )  // Service DNS name is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_DNS_NAME_IN_USE                                            IDS_ERROR_MESSAGE( 166 )  // Service DNS name already in use by another service.
#define IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS                          IDS_ERROR_MESSAGE( 167 )  // Compose deployment already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND                               IDS_ERROR_MESSAGE( 168 )  // Compose deployment not found.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_FOR_STATEFUL_SERVICES                              IDS_ERROR_MESSAGE( 169 )  // Operation only valid for stateless services.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_FOR_STATELESS_SERVICES                             IDS_ERROR_MESSAGE( 170 )  // Operation only valid for stateful services.
#define IDS_ERROR_MESSAGE_FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES                IDS_ERROR_MESSAGE( 171 )  // Operation only valid for stateful persistent services.
#define IDS_ERROR_MESSAGE_FABRIC_E_RA_NOT_READY_FOR_USE                                       IDS_ERROR_MESSAGE( 172 )  // Node is not ready to process this message.
#define IDS_ERROR_MESSAGE_FABRIC_E_INVALID_UPLOAD_SESSION_ID                                  IDS_ERROR_MESSAGE( 173 )  // The upload session identity is invalid.
#define IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS                 IDS_ERROR_MESSAGE( 174 )  // Single instance application already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND                      IDS_ERROR_MESSAGE( 175 )  // Single instance application not found.
#define IDS_ERROR_MESSAGE_FABRIC_E_CONTAINER_NOT_FOUND                                        IDS_ERROR_MESSAGE( 176 )  // Container not found.
#define IDS_ERROR_MESSAGE_FABRIC_E_VOLUME_ALREADY_EXISTS                                      IDS_ERROR_MESSAGE( 177 )  // Volume already exists.
#define IDS_ERROR_MESSAGE_FABRIC_E_VOLUME_NOT_FOUND                                           IDS_ERROR_MESSAGE( 178 )  // Volume not found.
#define IDS_ERROR_MESSAGE_FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC                             IDS_ERROR_MESSAGE( 179 )  // Central Secret Service generic error.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_FORCE_ON_ADHOC_SERVICE_REPLICA           IDS_ERROR_MESSAGE( 180 )  // Cannot close replica using Force for AdHoc type replica.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_FOR_NON_EXISTENT_FAILOVER_UNIT                IDS_ERROR_MESSAGE( 181 )  // Cannot close replica as provided partition does not exist.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_TRANSIENT_WITH_FORCE                          IDS_ERROR_MESSAGE( 182 )  // Force parameter not supported for restarting replica.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_TRANSIENT_FOR_NON_PERSISTED_SERVICE_REPLICA   IDS_ERROR_MESSAGE( 183 )  // Non persisted replica cannot be restarted. Please remove this replica.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_REPLICAID_MISMATCH                       IDS_ERROR_MESSAGE( 184 )  // Cannot close replica as provided ReplicaId do not match.
#define IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_INVALID_REPLICA_STATE                    IDS_ERROR_MESSAGE( 185 )  // Cannot close replica that is not open or has close in progress.
#define IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS            IDS_ERROR_MESSAGE( 186 )  // Single Instance application upgrade in progress.
#define IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING                           IDS_ERROR_MESSAGE( 187 )  // Compose deployment is not upgrading.

#define IDS_MODELV2( Index )                                        RESOURCE_ID( 26000, Index )
#define IDS_MODELV2_ContainerImageNotSpecified                      IDS_MODELV2( 1 ) // '{0}': 'image' not specified.
#define IDS_MODELV2_ContainerResourceRequestMemoryNotValid          IDS_MODELV2( 2 ) // '{0}': Resource request 'memory' not specified.
#define IDS_MODELV2_ContainerResourceRequestCpuNotValid             IDS_MODELV2( 3 ) // '{0}': Resource request 'cpu' not specified.
#define IDS_MODELV2_ContainerRegistryPasswordNotSpecified           IDS_MODELV2( 4 ) // '{0}': Container registry password not specified.
#define IDS_MODELV2_CodePackageNotSpecified                         IDS_MODELV2( 5 ) // '{0}': At least one code package must be specified.
#define IDS_MODELV2_ContainerEndpointNameNotSpecified               IDS_MODELV2( 6 ) // '{0}': Endpoint name not specified.
#define IDS_MODELV2_NameNotSpecified                                IDS_MODELV2( 7 ) // '{0}': 'name' not specified.
#define IDS_MODELV2_VolumeDestinationPathInvalidFormat              IDS_MODELV2( 8 ) // '{0}': Format of the 'destinationPath' '{1}' in volumes/volumeRefs is invalid.
#define IDS_MODELV2_ContainerImageTagNotSpecified                   IDS_MODELV2( 9 ) // '{0}': Image tag not specified.
#define IDS_MODELV2_EndpointNameNotUnique                           IDS_MODELV2( 10 ) // '{0}': Endpoint name '{1}' not unique.
#define IDS_MODELV2_ServiceNameNotUnique                            IDS_MODELV2( 11 ) // '{0}': Service name '{1}' not unique.
#define IDS_MODELV2_CodePackageNameNotUnique                        IDS_MODELV2( 12 ) // '{0}': CodePackage name '{1}' not unique.
#define IDS_MODELV2_ValueNotSpecified                               IDS_MODELV2( 13 ) // '{0}': 'value' not specified.
#define IDS_MODELV2_ApplicationScopedVolumeKindInvalid              IDS_MODELV2( 14 ) // '{0}': Application-scoped volume kind is invalid.
#define IDS_MODELV2_VolumeNameNotSpecified                          IDS_MODELV2( 15 ) // '{0}': Volume name not specified in volumes/volumeRefs
#define IDS_MODELV2_VolumeCreationParametersNotSpecified            IDS_MODELV2( 16 ) // '{0}': 'creationParameters' not specified in application-scoped volume ref.
#define IDS_MODELV2_InvalidCharactersInName                         IDS_MODELV2( 17 ) // '{0}': characters '{1}' are not allowed in name '{2}'
#define IDS_MODELV2_ServiceNotSpecified                             IDS_MODELV2( 18 ) // '{0}': At least one service description should be specified.
#define IDS_MODELV2_TriggerNotSupported                             IDS_MODELV2( 19 ) // '{0}': Only AverageLoad trigger kind is supported in this version.
#define IDS_MODELV2_MechanismNotSupported                           IDS_MODELV2( 20 ) // '{0}': Only AddRemoveReplica mechanism kind is supported in this version.
#define IDS_MODELV2_AutoScalingPolicy_MinMaxReplicas                IDS_MODELV2( 21 ) // Minimum number of replicas {0} cannot be greater than maximum number of replicas {1}.
#define IDS_MODELV2_AutoScalingPolicy_MinReplicaCount               IDS_MODELV2( 22 ) // Minimum replica count {0} must be greater than or equal to zero.
#define IDS_MODELV2_AutoScalingPolicy_MaxReplicaCount               IDS_MODELV2( 23 ) // Maximum replica count {0} must be either equal to -1 (unlimited) or greater than zero.
#define IDS_MODELV2_InvalidNetworkType                              IDS_MODELV2( 24 ) // '{0}': NetworkType {1} is invalid or not supported.
#define IDS_MODELV2_InvalidNetworkProperties                        IDS_MODELV2( 25 ) // '{0}': Network properties are invalid for networkType {1}.
#define IDS_MODELV2_HttpHostsNotSpecified                           IDS_MODELV2( 26 ) // '{0}': HttpConfig hosts not specified.
#define IDS_MODELV2_ServiceNameNotSpecified                         IDS_MODELV2( 27 ) // '{0}': Service name not specified.
#define IDS_MODELV2_ApplicationNameNotSpecified                     IDS_MODELV2( 28 ) // '{0}': Application name not specified.
#define IDS_MODELV2_PortNotSpecified                                IDS_MODELV2( 29 ) // '{0}': Listen Port not specified.
#define IDS_MODELV2_HttpRoutesNotSpecified                          IDS_MODELV2( 30 ) // '{0}': Http config routes not specified.
#define IDS_MODELV2_PathMatchTypeNotSpecified                       IDS_MODELV2( 31 ) // '{0}': Http path match type not specified.
#define IDS_MODELV2_PortNotUnique                                   IDS_MODELV2( 32 ) // '{0}': Port '{1}' not unique.
#define IDS_MODELV2_EndpointNotReferenced                           IDS_MODELV2( 33 ) // '{0}': Endpoint '{1}' not referenced in networkRefs.
