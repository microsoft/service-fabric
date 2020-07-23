// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{

#define CLIENT_STRUCTURED_QUERY_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::Client, \
                trace_id, \
                trace_type, \
                Common::LogLevel::trace_level, \
                Common::TraceChannelType::Debug, \
                TRACE_KEYWORDS2(Default, ForQuery), \
                __VA_ARGS__)

#define DECLARE_CLIENT_TRACE( traceName, ... ) \
    DECLARE_STRUCTURED_TRACE( traceName, __VA_ARGS__ );

#define CLIENT_TRACE( trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, Client, trace_id, trace_level, __VA_ARGS__),

#define CLIENT_QUERY_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
    CLIENT_STRUCTURED_QUERY_TRACE(trace_name, trace_id, trace_type, trace_level, __VA_ARGS__),

#define DECLARE_CLIENT_QUERY_TRACE( traceName, ... ) \
    DECLARE_STRUCTURED_TRACE( traceName, __VA_ARGS__ );

    class ClientEventSource
    {
        BEGIN_STRUCTURED_TRACES(ClientEventSource)

        CLIENT_TRACE( Open, 10, Info, "Opened successfully", "id" )
        CLIENT_TRACE( OpenFailed, 11, Warning, "Open failed: operation={1}, error={2}", "id", "operation", "error" )
        CLIENT_TRACE( Close, 12, Info, "Closed successfully", "id" )
        CLIENT_TRACE( CloseFailed, 13, Info, "Close failed: operation={1}, error={2}", "id", "operation", "error" )
        CLIENT_TRACE( Abort, 14, Info, "Aborted", "id" )
        CLIENT_TRACE( BeginCreateService, 15, Info, "{1}: partition description = {2}", "id", "activityId", "service")
        CLIENT_TRACE( BeginCreateServiceNameValidationFailure, 16, Info, "Service name {1} is not a valid NamingUri.", "id", "name")
        CLIENT_TRACE( BeginUpdateService, 17, Info, "{1}: name = {2} update description = {3}", "id", "activityId", "uri", "update")
        CLIENT_TRACE( BeginDeleteService, 18, Info, "{1}: BeginDeleteService for Service {2}", "id", "activityId", "description")
        CLIENT_TRACE( BeginCreateServiceFromTemplate, 19, Info, "{1}: appname = {2}, service name = {2}, type = {3}", "id", "activityId", "appName", "serviceName", "type")
        CLIENT_TRACE( BeginCreateName, 20, Info, "{1}: name = {2}", "id", "activityId", "name")
        CLIENT_TRACE( BeginDeleteName, 21, Info, "{1}: name = {2}", "id", "activityId", "name")
        CLIENT_TRACE( BeginNameExists, 22, Info, "{1}: name = {2}", "id", "activityId", "name")
        CLIENT_TRACE( BeginGetServiceDescription, 23, Info, "{1}: name = {2}", "id", "activityId", "name")
        CLIENT_TRACE( BeginEnumerateSubNames, 24, Info, "{1}: name = {2} recursive = {3}", "id", "activityId", "name", "recursive")
        CLIENT_TRACE( BeginSubmitPropertyBatch, 25, Info, "{1}: batch = {2}", "id", "activityId", "batch")
        CLIENT_TRACE( BeginEnumerateProperties, 26, Info, "{1}: name = {2} includeValues = {3}", "id", "activityId", "name", "includeValues")
        CLIENT_TRACE( BeginDeactivateNode, 27, Info, "{1}: node name = {2}", "id", "activityId", "nodeName")
        CLIENT_TRACE( BeginActivateNode, 28, Info, "{1}: node name = {2}", "id", "activityId", "nodeName")
        CLIENT_TRACE( BeginNodeStateRemoved, 29, Info, "{1}: node name = {2}", "id", "activityId", "nodeName")
        CLIENT_TRACE( BeginInternalQuery, 30, Info, "{1}: Query Name: {2}, Arguments: {3}", "id", "activityId", "queryName", "arguments")
        CLIENT_TRACE( BeginProvisionApplicationType, 31, Info, "{1}: BeginProvisionApplicationType for ApplicationType with buildPath='{2}', downloadUri='{3}'.", "id", "activityId", "applicationType", "downloadUri")
        CLIENT_TRACE( BeginCreateApplication, 32, Info, "{1}: BeginCreateApplication for Application {2}", "id", "activityId", "applicationNameUri")
        CLIENT_TRACE( BeginDeleteApplication, 33, Info, "{1}: BeginDeleteApplication for Application {2}", "id", "activityId", "description")
        CLIENT_TRACE( BeginUpgradeApplication, 34, Info, "{1}: BeginUpgradeApplication for Application {2}", "id", "activityId", "applicationNameUri")
        CLIENT_TRACE( BeginGetApplicationUpgradeProgress, 35, Info, "{1}: BeginGetApplicationUpgradeProgress for Application {2}", "id", "activityId", "applicationNameUri")
        CLIENT_TRACE( BeginUnprovisionApplicationType, 36, Info, "{1}: BeginUnprovisionApplicationType for '{2}' version '{3}'", "id", "activityId", "typeName", "typeVersion")
        CLIENT_TRACE( BeginMoveNextApplicationUpgradeDomain, 37, Info, "{1}: BeginMoveNextApplicationUpgradeDomain for Application {2}", "id", "activityId", "applicationNameUri")
        CLIENT_TRACE( BeginMoveNextApplicationUpgradeDomain2, 38, Info, "{1}: BeginMoveNextApplicationUpgradeDomain2 for Application {2}: UD = {3}", "id", "activityId", "applicationNameUri", "nextUpgradeDomain")
        CLIENT_TRACE( BeginProvisionFabric, 39, Info, "{1}: BeginProvisionFabric for CodeFilePath {2}, ClusterManifestFilePath {3}", "id", "activityId", "codeFilePath", "ClusterManifestFilePath")
        CLIENT_TRACE( BeginUpgradeFabric, 40, Info, "{1}: BeginUpgradeFabric FabricVersion:{2}", "id", "activityId", "FabricVersion")
        CLIENT_TRACE( BeginGetFabricUpgradeProgress, 41, Info, "{1}: BeginGetFabricUpgradeProgress", "id", "activityId")
        CLIENT_TRACE( BeginMoveNextFabricUpgradeDomain, 42, Info, "{1}: BeginMoveNextFabricUpgradeDomain with InProgressDomain {2}", "id", "activityId", "inProgressDomain")
        CLIENT_TRACE( BeginUnprovisionFabric, 43, Info, "{1}: BeginUnprovisionFabric for CodeVersion {2}, ConfigVersion {3}", "id", "activityId", "codeVersion", "configVersion")
        CLIENT_TRACE( BeginMoveNextFabricUpgradeDomain2, 44, Info, "{1}: BeginMoveNextFabricUpgradeDomain2: UD = {2}", "id", "activityId", "nextUpgradeDomain")
        CLIENT_TRACE( BeginStartInfrastructureTask, 45, Info, "{1}: BeginStartInfrastructureTask: {2}", "id", "activityId", "task")
        CLIENT_TRACE( BeginFinishInfrastructureTask, 46, Info, "{1}: BeginFinishInfrastructureTask: {2}", "id", "activityId", "taskInstance")
        CLIENT_TRACE( EstablishConnectionTimeout, 47, Info, "{1}: establish connection failed. Last ping address = {2}, original timeout = {3} msec", "id", "activityId", "gatewayAddress", "timeout")
        CLIENT_TRACE( SendRequest, 48, Info, "{1}: Sending request {2} to gateway {3}: timeout = {4}.", "id", "activityId", "action", "address", "timeout")
        CLIENT_TRACE( RequestComplete, 49, Noise, "{1}: Request to gateway {2} completed with {3}.", "id", "activityId", "address", "error")
        CLIENT_TRACE( BeginRecoverPartitions, 50, Info, "{1}", "id", "activityId")
        CLIENT_TRACE( RequestReplyCtor, 51, Noise, "{1}: RequestReplyOp constructor, timeout = {2}.", "id", "activityId", "timeout")
        CLIENT_TRACE( EndRequestFail, 52, Info, "{1}: failed: error = {2}", "id", "activityId", "error")
        CLIENT_TRACE( EstablishConnectionFail, 53, Info, "{1}: establish connection failed: error = {2}", "id", "activityId", "error")
        CLIENT_TRACE( EndRequestSuccess, 54, Noise, "{1}: succeeded", "id", "activityId")
        CLIENT_TRACE( CacheUpdate, 55, Noise, "{1}: Update cache with {2}", "id", "activityId", "rsp")
        CLIENT_TRACE( CacheRemoveEntry, 56, Noise, "{1}: Remove {2}, {3} (found {4}) due to {5}", "id", "activityId", "serviceName", "partitionInfo", "found", "error")
        CLIENT_TRACE( CacheRemoveName, 57, Noise, "{1}: Clear cache entries for {2}", "id", "activityId", "serviceName")
        CLIENT_TRACE( CacheUpdateWOverflow, 58, Noise, "{1}: Update cache with {2}, removed {3}", "id", "activityId", "rsp", "removed")
        CLIENT_TRACE( ResolveResultCoversPrevious, 59, Noise, "{1}: Result {2}.CompareTo(previous)={3}; primary request version={4}, current request={5}", "id", "activityId", "result", "compareResult", "primaryVersion", "crtVersion")
        CLIENT_TRACE( ResolveInProgress, 60, Info, "{1}: Partition {2} on service {3} is pending resolve.", "id", "activityId", "partition", "name")
        CLIENT_TRACE( ResolveServiceUseCached, 61, Noise, "{1}: Use cached location {2}", "id", "activityId", "cached")
        CLIENT_TRACE( BeginResolveService, 62, Info, "{1}: name={2} partition={3} prefix={4} !cached={5}", "id", "activityId", "name", "partition", "prefix", "bypass")
        CLIENT_TRACE( BeginResolveServiceWPrev, 63, Info, "{1}: name={2} partition={3} prevRsp=(gen={4}, fmVer={5}, psdVer={6}) prefix={7} !cached={8}", "id", "activityId", "name", "partition", "generation", "fmVer", "psdVer", "prefix", "bypass")
        CLIENT_TRACE( EndResolveServiceSuccess, 64, Info, "{1}: gen={2} fmVer={3} psdVer={4}", "id", "activityId", "generation", "fmVer", "psdVer" )
        CLIENT_TRACE( NotificationUseCached, 65, Noise, "{1}: Use cached entries", "id", "activityId")
        CLIENT_TRACE( NotificationPollGateway, 66, Noise, "{1}: Polling Gateway for service location changes", "id", "activityId")
        CLIENT_TRACE( RSPHasNoName, 67, Noise, "{1}: TryGetName failed for {2}", "id", "activityId", "rsp")
        CLIENT_TRACE( TrackerConstructor, 68, Info, "Create tracker for {1} (request {2}), partitionKey {3}, refAddr={4}", "id", "name", "requestName", "partitionKey", "refAddr")
        CLIENT_TRACE( TrackerAddRequest, 69, Info, "{1}: Add callback {2} to {3}", "id", "activityId", "handlerId", "tracker")
        CLIENT_TRACE( TrackerUpdateRSP, 70, Info, "{1}: Update {2} with {3}", "id", "activityId", "tracker", "rsp")
        CLIENT_TRACE( TrackerRemoveRequest, 71, Info, "Remove callback {1} from {2}", "id", "handlerId", "tracker")
        CLIENT_TRACE( TrackerStartPoll, 72, Info, "{1}: Start poll {2} with {3} requests, contentSize={4} (max={5})", "id", "activityId", "pollIndex", "requestCount", "size", "maxSize")
        CLIENT_TRACE( TrackerEndPoll, 73, Info, "{1}: End poll: {2}", "id", "activityId", "error")
        CLIENT_TRACE( TrackerDuplicateUpdate, 74, Noise, "{1}: Duplicate update at {2}", "id", "activityId", "serviceName")
        CLIENT_TRACE( TrackerRaiseCallbacks, 75, Noise, "{1}: {2} raise callbacks, startThread={3}", "id", "activityId", "handlers", "startThread")
        CLIENT_TRACE( TrackerRSPNotRelevant, 76, Noise, "{1}: {2} trackers: {3} not relevant", "id", "activityId", "trackerCount", "rsp")
        CLIENT_TRACE( TrackerCancelAll, 77, Info, "Cancel {1} trackers", "id", "trackerCount")
        CLIENT_TRACE( TrackerRaiseCallback, 78, Info, "{1}: Raise handler {2}", "id", "activityId", "callbackId")
        CLIENT_TRACE( TrackerDestructor, 79, Info, "{0}: Destruct tracker", "refAddr")
        CLIENT_TRACE( TrackerADFNotRelevant, 80, Noise, "{1}: {2} trackers: {3} not relevant", "id", "activityId", "trackerCount", "failure")
        CLIENT_TRACE( TrackerUpdateADF, 81, Info, "{1}: Update {2} with {3}", "id", "activityId", "tracker", "failure")
        CLIENT_TRACE( TrackerMsgSizeLimit, 82, Info, "{1}: Poll {2}: Can't add {3} because limit reached, size={4}, count={5}", "id", "activityId", "pollIndex", "request", "msgSize", "count")
        CLIENT_TRACE( TrackerTrace, 83, Info, "Tracker({1}, pk={2}, {3}", "contextSequenceId", "name", "pk", "callbacks")
        CLIENT_TRACE( TrackerCallbacksWithRSPTrace, 84, Info, "handlers=({1} last={2})", "contextSequenceId", "handlers", "rsp")
        CLIENT_TRACE( TrackerCallbacksWithADFTrace, 85, Info, "handlers=({1} last={2})", "contextSequenceId", "handlers", "failure")
        CLIENT_TRACE( TrackerCallbacksTrace, 86, Info, "handlers=({1})", "contextSequenceId", "handlers")
        CLIENT_TRACE( HandlerIdTrace, 87, Info, "{1};", "contextSequenceId", "handlerId")
        CLIENT_TRACE( BeginRecoverPartition, 88, Info, "{1}: PartitionId = {2}", "id", "activityId", "partitionId")
        CLIENT_TRACE( BeginRecoverServicePartitions, 89, Info, "{1}: ServiceName = {2}", "id", "activityId", "serviceName")
        CLIENT_TRACE( BeginRecoverSystemPartitions, 90, Info, "{1}", "id", "activityId")
        CLIENT_TRACE( BeginGetMetadata, 91, Info, "{1}", "id", "activityId")
        CLIENT_TRACE( BeginValidateToken, 92, Noise, "{1}", "id", "activityId")
        CLIENT_TRACE( BeginGetStagingLocation, 93, Info, "{1}: partitionId = {2}", "id", "activityId", "partitionId")
        CLIENT_TRACE( EndGetStagingLocation, 94, Info, "{1}: Error = {2}, StagingLocation = {3}", "id", "activityId", "error", "stagingLocation")
        CLIENT_TRACE( BeginUpload, 95, Info, "{1}: StagingRelativePath = {2}, StoreRelativePath = {3}, ShouldOverwrite = {4}, PartitionId = {5}", "id", "activityId", "stagingRelativePath", "storeRelativePath", "shouldOverwrite", "partitionId")
        CLIENT_TRACE( EndUpload, 96, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE( BeginListFiles, 97, Info, "{1}: StoreRelativePath = {2}, ContinuationToken = {3}, IsRecursive = {4}, IsPaging = {5}, PartitionId = {6}", "id", "activityId", "storeRelativePath", "continuationToken", "isRecursive", "isPaging", "partitionId")
        CLIENT_TRACE( EndListFiles, 98, Info, "{1}: Error = {2}, ContinuationToken = {3}, AvailableFileCount = {4}, AvailableFolderCount = {5}, AvailableShareCount = {6}", "id", "activityId", "error", "ContinuationToken", "availableFileCount", "availableFolderCount", "availableShareCount")
        CLIENT_TRACE( BeginDelete, 99, Info, "{1}: StoreRelativePath = {2}, PartitionId = {3}", "id", "activityId", "storeRelativePath", "partitionId")
        CLIENT_TRACE( EndDelete, 100, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE( BeginInternalGetStoreLocation, 101, Info, "{1}: ServiceName = {2}, PartitionId = {3}", "id", "activityId", "serviceName", "partitionId")
        CLIENT_TRACE( EndInternalGetStoreLocation, 102, Info, "{1}: Error = {2}, StoreShareLocation = {3}", "id", "activityId", "error", "storeShareLocation")
        CLIENT_TRACE( BeginInternalListFile, 103, Info, "{1}: StoreRelativePath = {2}, ServiceName = {3}, PartitionId = {4}", "id", "activityId", "storeRelativePath", "serviceName", "partitionId")
        CLIENT_TRACE( EndInternalListFile, 104, Info, "{1}: Error = {2}, IsPresent = {3}, CurrentState = {4}, CurrentVersion = {5}, StoreShareLocation = {6}", "id", "activityId", "error", "isPresent", "currentState", "currentVersion", "storeShareLocation")
        CLIENT_QUERY_TRACE( BeginReportFault, 105, "_Client_BeginReportFault", Info, "{1}: {2}", "id", "activityId", "body")
        CLIENT_TRACE( EndReportFault, 106, Info, "{1}: {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginSetAcl, 107, Info, "{1}: name = {2}, acl = {3}", "id", "activityId", "name", "acl")
        CLIENT_TRACE(BeginGetAcl, 108, Info, "{1}: name = {2}", "id", "activityId", "name")
        CLIENT_TRACE(BeginCopy, 109, Info, "{1}: SouceStoreRelativePath = {2}, SourceVersion = {3}, DestinationStoreRelativePath = {4}, ShouldOverwrite = {5}, PartitionId = {6}", "id", "activityId", "sourceStoreRelativePath", "sourceVersion", "destinationStoreRelativePath", "shouldOverwrite", "partitionId")
        CLIENT_TRACE(EndCopy, 110, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginResolveSystemService, 111, Noise, "{1}: name={2} partition={3}", "id", "activityId", "name", "partition")
        CLIENT_TRACE(EndResolveSystemServiceSuccess, 112, Noise, "{1}: gen={2} fmVer={3} psdVer={4}", "id", "activityId", "generation", "fmVer", "psdVer")
        CLIENT_TRACE(EndResolveSystemServiceFailure, 113, Noise, "{1}: error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginInternalGetStoreLocations, 114, Info, "{1}: PartitionId = {2}", "id", "activityId", "partitionId")
        CLIENT_TRACE(EndInternalGetStoreLocations, 115, Info, "{1}: Error = {2} Shares = {3}", "id", "activityId", "error", "locations")

        // client PSD+RSP cache
        //
        CLIENT_TRACE( PsdCacheMiss, 120, Noise, "{1}: {2} prefix={3}", "id", "activityId", "name", "prefix")
        CLIENT_TRACE( PsdCacheInvalidation, 121, Info, "{1}: cache({2}, psdVer={3})", "id", "activityId", "name", "psdVer")
        CLIENT_TRACE( PsdCacheProcessing, 122, Noise, "{1}: cache({2}, psdVer={3}) request(psdVer={4})", "id", "activityId", "name", "psdVer", "requestPsdVer")
        CLIENT_TRACE( PsdCacheRefreshed, 123, Noise, "{1}: cache({2}, psdVer={3}) request(psdVer={4})", "id", "activityId", "name", "psdVer", "requestPsdVer")
        CLIENT_TRACE( RspCacheMiss, 124, Info, "{1}: {2} request(key={3}, {4})", "id", "activityId", "name", "requestKey", "requestVersion")
        CLIENT_TRACE( RspCacheInvalidation, 125, Info, "{1}: cache(cuid={2}, gen={3}, fmVer={4}, psdVer={5}) request(key={6}, {7})", "id", "activityId", "cuid", "generation", "fmVer", "psdVer", "requestKey", "requestVer")
        CLIENT_TRACE( RspCacheProcessing, 126, Noise, "{1}: cache(cuid={2}, gen={3}, fmVer={4}, psdVer={5}) request(key={6}, {7})", "id", "activityId", "cuid", "generation", "fmVer", "psdVer", "requestKey", "requestVer")
        CLIENT_TRACE( NotificationCacheUpdate, 127, Noise, "{1}: cache(psdVer={2}) rsp(cuid={3}, gen={4}, fmVer={5}, psdVer={6})", "id", "activityId", "psdVer", "cuid", "generation", "fmVer", "rspPsdVer")
        CLIENT_TRACE( NotificationCacheUpdateOnRefresh, 128, Noise, "{1}: cache(psdVer={2}) rsp(cuid={3}, gen={4}, fmVer={5}, psdVer={6})", "id", "activityId", "psdVer", "cuid", "generation", "fmVer", "rspPsdVer")

        CLIENT_TRACE( EndResolveServiceFailure, 129, Info, "{1}: error = {2}", "id", "activityId", "error" )

        CLIENT_TRACE( BeginUpdateApplicationUpgrade, 130, Info, "{1}: application = {2}", "id", "activityId", "application" )
        CLIENT_TRACE( BeginUpdateFabricUpgrade, 131, Info, "{1}", "id", "activityId" )
        CLIENT_TRACE( BeginStartNode, 132, Info, "{1}: Node = {2}:{3} IpAddressOrFQDN = {4}, ClientConnectionPort = {5}", "id", "activityId", "node", "instanceId", "ipAddressOrFQDN", "clientConnectionPort")
        CLIENT_TRACE( BeginRollbackApplicationUpgrade, 133, Info, "{1}: application = {2}", "id", "activityId", "application" )
        CLIENT_TRACE( BeginRollbackFabricUpgrade, 134, Info, "{1}", "id", "activityId" )

        CLIENT_TRACE( BeginCreateRepairRequest, 140, Info, "{1}: repairTask = {2}", "id", "activityId", "repairTask")
        CLIENT_TRACE( EndCreateRepairRequest, 141, Info, "{1}: error = {2}, commitVersion = {3}", "id", "activityId", "error", "commitVersion")
        CLIENT_TRACE( BeginCancelRepairRequest, 142, Info, "{1}: repairTaskId = {2}, version = {3}, abort = {4}", "id", "activityId", "repairTaskId", "version", "abort")
        CLIENT_TRACE( EndCancelRepairRequest, 143, Info, "{1}: error = {2}, commitVersion = {3}", "id", "activityId", "error", "commitVersion")
        CLIENT_TRACE( BeginForceApproveRepair, 144, Info, "{1}: repairTaskId = {2}, version = {3}", "id", "activityId", "repairTaskId", "version")
        CLIENT_TRACE( EndForceApproveRepair, 145, Info, "{1}: error = {2}, commitVersion = {3}", "id", "activityId", "error", "commitVersion")
        CLIENT_TRACE( BeginDeleteRepairRequest, 146, Info, "{1}: repairTaskId = {2}, version = {3}", "id", "activityId", "repairTaskId", "version")
        CLIENT_TRACE( EndDeleteRepairRequest, 147, Info, "{1}: error = {2}", "id", "activityId", "error")
        CLIENT_TRACE( BeginUpdateRepairExecutionState, 148, Info, "{1}: repairTask = {2}", "id", "activityId", "repairTask")
        CLIENT_TRACE( EndUpdateRepairExecutionState, 149, Info, "{1}: error = {2}, commitVersion = {3}", "id", "activityId", "error", "commitVersion")
        CLIENT_TRACE( BeginUpdateApplication, 150, Info, "{1}: BeginUpdateApplication for Application {2}", "id", "activityId", "applicationNameUri")
        CLIENT_TRACE( BeginUpdateRepairTaskHealthPolicy, 151, Info, "{1}: repairTaskId = {2}, version = {3}", "id", "activityId", "repairTaskId", "version" )
        CLIENT_TRACE( EndUpdateRepairTaskHealthPolicy, 152, Info, "{1}: error = {2}, commitVersion = {3}", "id", "activityId", "error", "commitVersion" )
        CLIENT_TRACE(BeginListUploadSession, 153, Info, "{1}: StoreRelativePath = {2}, SessionId = {3}, PartitionId = {4}", "id", "activityId", "storeRelativePath", "sessionId", "partitionId")
        CLIENT_TRACE(EndListUploadSession, 154, Info, "{1}: Error = {2}, SessionCount = {3}", "id", "activityId", "error", "sessionCount")
        CLIENT_TRACE(BeginCreateUploadSession, 155, Info, "{1}: StoreRelativePath = {2}, SessionId = {3}, FileSize = {4}, PartitionId = {5}", "id", "activityId", "storeRelativePath", "sessionId", "fileSize", "partitionId")
        CLIENT_TRACE(EndCreateUploadSession, 156, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginUploadChunk, 157, Info, "{1}: LocalSource = {2}, SessionId = {3}, StartPosition = {4}, EndPosition = {5}, PartitionId = {6}", "id", "activityId", "localSource", "sessionId", "startPosition", "endPosition", "partitionId")
        CLIENT_TRACE(EndUploadChunk, 158, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginDeleteUploadSession, 159, Info, "{1}: SessionId = {2}, PartitionId = {3}", "id", "activityId", "sessionId", "partitionId")
        CLIENT_TRACE(EndDeleteUploadSession, 160, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginCommitUploadSession, 161, Info, "{1}: SessionId = {2}, PartitionId = {3}", "id", "activityId", "sessionId", "partitionId")
        CLIENT_TRACE(EndCommitUploadSession, 162, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginCheckExistence, 163, Info, "{1}: StoreRelativePath = {2}", "id", "activityId", "storeRelativePath")
        CLIENT_TRACE(EndCheckExistence, 164, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginCreateComposeDeployment, 165, Info, "{1}: DeploymentName = {2}, ComposeFilePath = {3}, OverridesFilepath = {4}", "id", "activityId", "deploymentName", "composeFilePath", "overridesFilePath")
        CLIENT_TRACE(BeginCreateComposeDeployment2, 166, Info, "{1}: DeploymentName = {2}", "id", "activityId", "deploymentName")
        CLIENT_TRACE(BeginDeleteComposeDeployment, 167, Info, "{1}: DeploymentName = {2}", "id", "activityId", "deploymentName")
        CLIENT_TRACE(AcceptedNotification, 170, Info, "[{1}] accepted service notification ({2}): {3}", "id", "pageId", "serviceName", "partition")
        CLIENT_TRACE(BeginUpgradeComposeDeployment, 171, Info, "{1}: DeploymentName = {2}, ComposeFiles Count = {3}, SFSettings files Count = {4}", "id", "activityId", "deploymentName", "composeFileCount", "sfSettingsFilesCount")
        CLIENT_TRACE(BeginRollbackComposeDeployment, 172, Info, "{1}: DeploymentName = {2}", "id", "activityId", "deploymentName")
        CLIENT_TRACE(BeginDeleteSingleInstanceDeployment, 173, Info, "{1}: Description = {2}", "id", "activityId", "description")
        CLIENT_TRACE(BeginCreateOrUpdateApplicationResource, 174, Info, "{1}: ApplicationName = {2}, Services = {3}", "id", "activityId", "applicationName", "serviceCount")
        CLIENT_TRACE(BeginCreateVolume, 175, Info, "{1}: VolumeName = {2}", "id", "activityId", "volumeName")
        CLIENT_TRACE(BeginDeleteVolume, 176, Info, "{1}: VolumeName = {2}", "id", "activityId", "volumeName")
        CLIENT_TRACE(BeginUploadChunkContent, 177, Info, "{1}: SessionId = {2}, StartPosition = {3}, EndPosition = {4}, Size= {5}, PartitionId = {6}", "id", "activityId", "sessionId", "startPosition", "endPosition", "size", "partitionId")
        CLIENT_TRACE(EndUploadChunkContent, 178, Info, "{1}: Error = {2}", "id", "activityId", "error")
        CLIENT_TRACE(BeginCreateNetwork, 179, Info, "{1}: BeginCreateNetwork for Network {2}", "id", "activityId", "networkName")
        CLIENT_TRACE(BeginDeleteNetwork, 180, Info, "{1}: BeginDeleteNetwork for Network {2}", "id", "activityId", "networkName")

        CLIENT_TRACE(BeginCreateOrUpdateGatewayResource, 181, Info, "{1} BeginCreateOrUpdateGatewayResource", "id", "activityId")
        CLIENT_TRACE(BeginDeleteGatewayResource, 182, Info, "{1}: BeginDeleteGatewayResource for Gateway {2}", "id", "activityId", "name")
        END_STRUCTURED_TRACES

        DECLARE_CLIENT_TRACE( Open, std::wstring )
        DECLARE_CLIENT_TRACE( OpenFailed, std::wstring, std::wstring, Common::ErrorCode )
        DECLARE_CLIENT_TRACE( Close, std::wstring )
        DECLARE_CLIENT_TRACE( CloseFailed, std::wstring, std::wstring, Common::ErrorCode )
        DECLARE_CLIENT_TRACE( Abort, std::wstring )
        DECLARE_CLIENT_TRACE( BeginCreateService, std::wstring, Common::ActivityId, Reliability::ServiceDescription )
        DECLARE_CLIENT_TRACE( BeginCreateServiceNameValidationFailure, std::wstring, std::wstring )
        DECLARE_CLIENT_TRACE( BeginUpdateService, std::wstring, Common::ActivityId, Common::Uri, Naming::ServiceUpdateDescription )
        DECLARE_CLIENT_TRACE( BeginDeleteService, std::wstring, Common::ActivityId, ServiceModel::DeleteServiceDescription )
        DECLARE_CLIENT_TRACE( BeginCreateServiceFromTemplate, std::wstring, Common::ActivityId, Common::Uri, Common::Uri, std::wstring )
        DECLARE_CLIENT_TRACE( BeginCreateName, std::wstring, Common::ActivityId, Common::Uri )
        DECLARE_CLIENT_TRACE( BeginDeleteName, std::wstring, Common::ActivityId, Common::Uri )
        DECLARE_CLIENT_TRACE( BeginNameExists, std::wstring, Common::ActivityId, Common::Uri )
        DECLARE_CLIENT_TRACE( BeginGetServiceDescription, std::wstring, Common::ActivityId, Common::Uri )
        DECLARE_CLIENT_TRACE( BeginEnumerateSubNames, std::wstring, Common::ActivityId, Common::Uri, bool )
        DECLARE_CLIENT_TRACE( BeginSubmitPropertyBatch, std::wstring, Common::ActivityId, Naming::NamePropertyOperationBatch )
        DECLARE_CLIENT_TRACE( BeginEnumerateProperties, std::wstring, Common::ActivityId, Common::Uri, bool )
        DECLARE_CLIENT_TRACE( BeginDeactivateNode, std::wstring, Common::ActivityId, std::wstring )
        DECLARE_CLIENT_TRACE( BeginActivateNode, std::wstring, Common::ActivityId, std::wstring )
        DECLARE_CLIENT_TRACE( BeginNodeStateRemoved, std::wstring, Common::ActivityId, std::wstring )
        DECLARE_CLIENT_TRACE( BeginInternalQuery, std::wstring, Common::ActivityId, std::wstring, ServiceModel::QueryArgumentMap )
        DECLARE_CLIENT_TRACE( BeginProvisionApplicationType, std::wstring, Common::ActivityId, std::wstring, std::wstring )
        DECLARE_CLIENT_TRACE( BeginCreateApplication, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginDeleteApplication, std::wstring, Common::ActivityId, ServiceModel::DeleteApplicationDescription )
        DECLARE_CLIENT_TRACE( BeginUpgradeApplication, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginGetApplicationUpgradeProgress, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginUnprovisionApplicationType, std::wstring, Common::ActivityId, std::wstring, std::wstring )
        DECLARE_CLIENT_TRACE( BeginMoveNextApplicationUpgradeDomain, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginMoveNextApplicationUpgradeDomain2, std::wstring, Common::ActivityId, Common::NamingUri, std::wstring )
        DECLARE_CLIENT_TRACE( BeginProvisionFabric, std::wstring, Common::ActivityId, std::wstring, std::wstring )
        DECLARE_CLIENT_TRACE( BeginUpgradeFabric, std::wstring, Common::ActivityId, Common::FabricVersion )
        DECLARE_CLIENT_TRACE( BeginGetFabricUpgradeProgress, std::wstring, Common::ActivityId )
        DECLARE_CLIENT_TRACE( BeginMoveNextFabricUpgradeDomain, std::wstring, Common::ActivityId, std::wstring )
        DECLARE_CLIENT_TRACE( BeginUnprovisionFabric, std::wstring, Common::ActivityId, Common::FabricCodeVersion, Common::FabricConfigVersion )
        DECLARE_CLIENT_TRACE( BeginMoveNextFabricUpgradeDomain2, std::wstring, Common::ActivityId, std::wstring )
        DECLARE_CLIENT_TRACE( BeginStartInfrastructureTask, std::wstring, Common::ActivityId, Management::ClusterManager::InfrastructureTaskDescription )
        DECLARE_CLIENT_TRACE( BeginFinishInfrastructureTask, std::wstring, Common::ActivityId, Management::ClusterManager::TaskInstance )
        DECLARE_CLIENT_TRACE( EstablishConnectionTimeout, std::wstring, Common::ActivityId, std::wstring, int64 )
        DECLARE_CLIENT_TRACE( SendRequest, std::wstring, Common::ActivityId, std::wstring, std::wstring, Common::TimeSpan )
        DECLARE_CLIENT_TRACE( RequestComplete, std::wstring, Common::ActivityId, std::wstring, Common::ErrorCode )
        DECLARE_CLIENT_TRACE( BeginRecoverPartitions, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( BeginRecoverPartition, std::wstring, Common::ActivityId, Common::Guid)
        DECLARE_CLIENT_TRACE( BeginRecoverServicePartitions, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginRecoverSystemPartitions, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( BeginGetMetadata, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( BeginValidateToken, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( RequestReplyCtor, std::wstring, Common::ActivityId, Common::TimeSpan)
        DECLARE_CLIENT_TRACE( EndRequestFail, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( EstablishConnectionFail, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( EndRequestSuccess, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( CacheUpdate, std::wstring, Common::ActivityId, Naming::ResolvedServicePartition);
        DECLARE_CLIENT_TRACE( CacheRemoveEntry, std::wstring, Common::ActivityId, std::wstring, Naming::PartitionInfo, bool, int)
        DECLARE_CLIENT_TRACE( CacheRemoveName, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( CacheUpdateWOverflow, std::wstring, Common::ActivityId, Naming::ResolvedServicePartition, Naming::NameRangeTuple)
        DECLARE_CLIENT_TRACE( ResolveResultCoversPrevious, std::wstring, Common::ActivityId, Naming::ResolvedServicePartition, int64, Naming::ServiceLocationVersion, Naming::ServiceLocationVersion)
        DECLARE_CLIENT_TRACE( ResolveInProgress, std::wstring, Common::ActivityId, Naming::PartitionKey, Common::Uri)
        DECLARE_CLIENT_TRACE( ResolveServiceUseCached, std::wstring, Common::ActivityId, Naming::ResolvedServicePartition)
        DECLARE_CLIENT_TRACE( BeginResolveService, std::wstring, Common::ActivityId, Common::Uri, Naming::PartitionKey, bool, bool)
        DECLARE_CLIENT_TRACE( BeginResolveServiceWPrev, std::wstring, Common::ActivityId, Common::Uri, Naming::PartitionKey, Reliability::GenerationNumber, int64, int64, bool, bool)
        DECLARE_CLIENT_TRACE( EndResolveServiceSuccess, std::wstring, Common::ActivityId, Reliability::GenerationNumber, int64, int64 )
        DECLARE_CLIENT_TRACE( BeginGetStagingLocation, std::wstring, Common::ActivityId, Common::Guid)
        DECLARE_CLIENT_TRACE( EndGetStagingLocation, std::wstring, Common::ActivityId, Common::ErrorCode, std::wstring)
        DECLARE_CLIENT_TRACE( BeginUpload, std::wstring, Common::ActivityId, std::wstring, std::wstring, bool, Common::Guid)
        DECLARE_CLIENT_TRACE( EndUpload, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginCopy, std::wstring, Common::ActivityId, std::wstring, std::wstring, std::wstring, bool, Common::Guid)
        DECLARE_CLIENT_TRACE( EndCopy, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE(BeginResolveSystemService, std::wstring, Common::ActivityId, Common::Uri, Naming::PartitionKey)
        DECLARE_CLIENT_TRACE(EndResolveSystemServiceSuccess, std::wstring, Common::ActivityId, Reliability::GenerationNumber, int64, int64)
        DECLARE_CLIENT_TRACE(EndResolveSystemServiceFailure, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginListFiles, std::wstring, Common::ActivityId, std::wstring, std::wstring, bool, bool, Common::Guid)
        DECLARE_CLIENT_TRACE( EndListFiles, std::wstring, Common::ActivityId, Common::ErrorCode, std::wstring, uint64, uint64, uint64)
        DECLARE_CLIENT_TRACE( BeginDelete, std::wstring, Common::ActivityId, std::wstring, Common::Guid)
        DECLARE_CLIENT_TRACE( EndDelete, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginInternalGetStoreLocation, std::wstring, Common::ActivityId, Common::NamingUri, Common::Guid)
        DECLARE_CLIENT_TRACE( EndInternalGetStoreLocation, std::wstring, Common::ActivityId, Common::ErrorCode, std::wstring)
        DECLARE_CLIENT_TRACE( BeginInternalListFile, std::wstring, Common::ActivityId, std::wstring, Common::NamingUri, Common::Guid)
        DECLARE_CLIENT_TRACE( EndInternalListFile, std::wstring, Common::ActivityId, Common::ErrorCode, bool, std::wstring, std::wstring, std::wstring)
        DECLARE_CLIENT_QUERY_TRACE( BeginReportFault, std::wstring, Common::ActivityId, Reliability::ReportFaultRequestMessageBody)
        DECLARE_CLIENT_TRACE( EndReportFault, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( NotificationUseCached, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( NotificationPollGateway, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( RSPHasNoName, std::wstring, Common::ActivityId, Naming::ResolvedServicePartition)
        DECLARE_CLIENT_TRACE( TrackerConstructor, std::wstring, std::wstring, std::wstring, Naming::PartitionKey, __int64);
        DECLARE_CLIENT_TRACE( TrackerAddRequest, std::wstring, Common::ActivityId, LONGLONG, ServiceAddressTracker);
        DECLARE_CLIENT_TRACE( TrackerUpdateRSP, std::wstring, Common::ActivityId, ServiceAddressTrackerCallbacks, Naming::ResolvedServicePartition);
        DECLARE_CLIENT_TRACE( TrackerUpdateADF, std::wstring, Common::ActivityId, ServiceAddressTrackerCallbacks, Naming::AddressDetectionFailure);
        DECLARE_CLIENT_TRACE( TrackerRemoveRequest, std::wstring, LONGLONG, ServiceAddressTracker);
        DECLARE_CLIENT_TRACE( TrackerStartPoll, std::wstring, Common::ActivityId, uint64, uint64, uint64, uint64);
        DECLARE_CLIENT_TRACE( TrackerEndPoll, std::wstring, Common::ActivityId, Common::ErrorCode);
        DECLARE_CLIENT_TRACE( TrackerDuplicateUpdate, std::wstring, Common::ActivityId, std::wstring);
        DECLARE_CLIENT_TRACE( TrackerRaiseCallbacks, std::wstring, Common::ActivityId, ServiceAddressTracker, bool);
        DECLARE_CLIENT_TRACE( TrackerRSPNotRelevant, std::wstring, Common::ActivityId, uint64, Naming::ResolvedServicePartition);
        DECLARE_CLIENT_TRACE( TrackerADFNotRelevant, std::wstring, Common::ActivityId, uint64, Naming::AddressDetectionFailure);
        DECLARE_CLIENT_TRACE( TrackerCancelAll, std::wstring, uint64);
        DECLARE_CLIENT_TRACE( TrackerRaiseCallback, std::wstring, Common::ActivityId, LONGLONG);
        DECLARE_CLIENT_TRACE( TrackerDestructor, __int64);
        DECLARE_CLIENT_TRACE( TrackerMsgSizeLimit, std::wstring, Common::ActivityId, uint64, Naming::ServiceLocationNotificationRequestData, uint64, uint64);
        DECLARE_CLIENT_TRACE( TrackerTrace, uint16, Common::Uri, Naming::PartitionKey, ServiceAddressTrackerCallbacks);
        DECLARE_CLIENT_TRACE( TrackerCallbacksWithRSPTrace, uint16, std::vector<ServiceAddressTrackerCallbacks::HandlerIdWrapper>, Naming::ResolvedServicePartition);
        DECLARE_CLIENT_TRACE( TrackerCallbacksWithADFTrace, uint16, std::vector<ServiceAddressTrackerCallbacks::HandlerIdWrapper>, Naming::AddressDetectionFailure);
        DECLARE_CLIENT_TRACE( TrackerCallbacksTrace, uint16, std::vector<ServiceAddressTrackerCallbacks::HandlerIdWrapper>);
        DECLARE_CLIENT_TRACE( HandlerIdTrace, uint16, LONGLONG);

        DECLARE_CLIENT_TRACE( BeginInternalGetStoreLocations, std::wstring, Common::ActivityId, Common::Guid)
        DECLARE_CLIENT_TRACE( EndInternalGetStoreLocations, std::wstring, Common::ActivityId, Common::ErrorCode, int64)

        DECLARE_CLIENT_TRACE( PsdCacheMiss, std::wstring, Common::ActivityId, Common::NamingUri, bool);
        DECLARE_CLIENT_TRACE( PsdCacheInvalidation, std::wstring, Common::ActivityId, Common::NamingUri, int64)
        DECLARE_CLIENT_TRACE( PsdCacheProcessing, std::wstring, Common::ActivityId, Common::NamingUri, int64, int64);
        DECLARE_CLIENT_TRACE( PsdCacheRefreshed, std::wstring, Common::ActivityId, Common::NamingUri, int64, int64);
        DECLARE_CLIENT_TRACE( RspCacheMiss, std::wstring, Common::ActivityId, Common::NamingUri, Naming::PartitionKey, Naming::ServiceLocationVersion);
        DECLARE_CLIENT_TRACE( RspCacheInvalidation, std::wstring, Common::ActivityId, Common::Guid, Reliability::GenerationNumber, int64, int64, Naming::PartitionKey, Naming::ServiceLocationVersion);
        DECLARE_CLIENT_TRACE( RspCacheProcessing, std::wstring, Common::ActivityId, Common::Guid, Reliability::GenerationNumber, int64, int64, Naming::PartitionKey, Naming::ServiceLocationVersion);
        DECLARE_CLIENT_TRACE( NotificationCacheUpdate, std::wstring, Common::ActivityId, int64, Common::Guid, Reliability::GenerationNumber, int64, int64 );
        DECLARE_CLIENT_TRACE( NotificationCacheUpdateOnRefresh, std::wstring, Common::ActivityId, int64, Common::Guid, Reliability::GenerationNumber, int64, int64 );

        DECLARE_CLIENT_TRACE( EndResolveServiceFailure, std::wstring, Common::ActivityId, Common::ErrorCode )

        DECLARE_CLIENT_TRACE( BeginUpdateApplicationUpgrade, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginUpdateFabricUpgrade, std::wstring, Common::ActivityId )

        DECLARE_CLIENT_TRACE( BeginStartNode, std::wstring, Common::ActivityId, std::wstring, uint64, std::wstring, uint64)
        DECLARE_CLIENT_TRACE( BeginRollbackApplicationUpgrade, std::wstring, Common::ActivityId, Common::NamingUri )
        DECLARE_CLIENT_TRACE( BeginRollbackFabricUpgrade, std::wstring, Common::ActivityId )

        DECLARE_CLIENT_TRACE( BeginCreateRepairRequest, std::wstring, Common::ActivityId, Management::RepairManager::RepairTask)
        DECLARE_CLIENT_TRACE( EndCreateRepairRequest, std::wstring, Common::ActivityId, Common::ErrorCode, int64)
        DECLARE_CLIENT_TRACE( BeginCancelRepairRequest, std::wstring, Common::ActivityId, std::wstring, int64, bool)
        DECLARE_CLIENT_TRACE( EndCancelRepairRequest, std::wstring, Common::ActivityId, Common::ErrorCode, int64)
        DECLARE_CLIENT_TRACE( BeginForceApproveRepair, std::wstring, Common::ActivityId, std::wstring, int64)
        DECLARE_CLIENT_TRACE( EndForceApproveRepair, std::wstring, Common::ActivityId, Common::ErrorCode, int64)
        DECLARE_CLIENT_TRACE( BeginDeleteRepairRequest, std::wstring, Common::ActivityId, std::wstring, int64)
        DECLARE_CLIENT_TRACE( EndDeleteRepairRequest, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginUpdateRepairExecutionState, std::wstring, Common::ActivityId, Management::RepairManager::RepairTask)
        DECLARE_CLIENT_TRACE( EndUpdateRepairExecutionState, std::wstring, Common::ActivityId, Common::ErrorCode, int64)
        DECLARE_CLIENT_TRACE( BeginUpdateRepairTaskHealthPolicy, std::wstring, Common::ActivityId, std::wstring, int64 )
        DECLARE_CLIENT_TRACE( EndUpdateRepairTaskHealthPolicy, std::wstring, Common::ActivityId, Common::ErrorCode, int64 )

        DECLARE_CLIENT_TRACE(BeginSetAcl, std::wstring, Common::ActivityId, Common::Uri, AccessControl::FabricAcl)
        DECLARE_CLIENT_TRACE(BeginGetAcl, std::wstring, Common::ActivityId, Common::Uri)

        DECLARE_CLIENT_TRACE(BeginUpdateApplication, std::wstring, Common::ActivityId, Common::NamingUri)

        DECLARE_CLIENT_TRACE( BeginListUploadSession, std::wstring, Common::ActivityId, std::wstring, Common::Guid, Common::Guid)
        DECLARE_CLIENT_TRACE( EndListUploadSession, std::wstring, Common::ActivityId, Common::ErrorCode, uint64)
        DECLARE_CLIENT_TRACE( BeginCreateUploadSession, std::wstring, Common::ActivityId, std::wstring, Common::Guid, uint64, Common::Guid)
        DECLARE_CLIENT_TRACE( EndCreateUploadSession, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginUploadChunk, std::wstring, Common::ActivityId, std::wstring, Common::Guid, uint64, uint64, Common::Guid)
        DECLARE_CLIENT_TRACE( EndUploadChunk, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginDeleteUploadSession, std::wstring, Common::ActivityId, Common::Guid, Common::Guid)
        DECLARE_CLIENT_TRACE( EndDeleteUploadSession, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginCommitUploadSession, std::wstring, Common::ActivityId, Common::Guid, Common::Guid)
        DECLARE_CLIENT_TRACE( EndCommitUploadSession, std::wstring, Common::ActivityId, Common::ErrorCode)

        DECLARE_CLIENT_TRACE( BeginCheckExistence, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( EndCheckExistence, std::wstring, Common::ActivityId, Common::ErrorCode)

        DECLARE_CLIENT_TRACE( BeginCreateComposeDeployment, std::wstring, Common::ActivityId, std::wstring, std::wstring, std::wstring)
        DECLARE_CLIENT_TRACE( BeginCreateComposeDeployment2, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginDeleteComposeDeployment, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( AcceptedNotification, std::wstring, Naming::ServiceNotificationPageId, std::wstring, Reliability::ServiceTableEntry)
        DECLARE_CLIENT_TRACE( BeginUpgradeComposeDeployment, std::wstring, Common::ActivityId, std::wstring, uint64, uint64)
        DECLARE_CLIENT_TRACE(BeginRollbackComposeDeployment, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginDeleteSingleInstanceDeployment, std::wstring, Common::ActivityId, ServiceModel::DeleteSingleInstanceDeploymentDescription )
        DECLARE_CLIENT_TRACE( BeginCreateOrUpdateApplicationResource, std::wstring, Common::ActivityId, std::wstring, size_t)
        DECLARE_CLIENT_TRACE( BeginCreateVolume, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginDeleteVolume, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginUploadChunkContent, std::wstring, Common::ActivityId, Common::Guid, uint64, uint64, uint64, Common::Guid);
        DECLARE_CLIENT_TRACE( EndUploadChunkContent, std::wstring, Common::ActivityId, Common::ErrorCode)
        DECLARE_CLIENT_TRACE( BeginCreateNetwork, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginDeleteNetwork, std::wstring, Common::ActivityId, std::wstring)
        DECLARE_CLIENT_TRACE( BeginCreateOrUpdateGatewayResource, std::wstring, Common::ActivityId)
        DECLARE_CLIENT_TRACE( BeginDeleteGatewayResource, std::wstring, Common::ActivityId, std::wstring)
    };
}
