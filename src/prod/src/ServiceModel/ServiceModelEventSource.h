// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{

#define DECLARE_SM_TRACE( traceName, ... ) \
    DECLARE_STRUCTURED_TRACE( traceName, __VA_ARGS__ );

#define SM_TRACE( trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, ServiceModel, trace_id, trace_level, __VA_ARGS__),

    class ServiceModelEventSource
    {
        BEGIN_STRUCTURED_TRACES( ServiceModelEventSource )

        SM_TRACE( NodeTaskDescription, 10, Info, "{1}:{2}", "id", "nodeName", "taskType" )
        SM_TRACE( NamePropPutOp, 11, Info, "({1}:{2}:{3}:'{4}')", "contextSequenceId", "opType", "property", "propType", "customTypeId")
        SM_TRACE( NamePropGetOp, 12, Info, "({1}:{2}:{3})", "contextSequenceId", "opType", "property", "includeValues")
        SM_TRACE( NamePropOp, 13, Info, "({1}:{2})", "contextSequenceId", "opType", "property")
        SM_TRACE( NamePropCheckExistsOp, 14, Info, "({1}:{2}:{3})", "contextSequenceId", "opType", "property", "exists")
        SM_TRACE( NamePropCheckSequenceOp, 15, Info, "({1}:{2}:{3})", "contextSequenceId", "opType", "property", "sequence")
        SM_TRACE( NamePropCheckValueOp, 16, Info, "({1}:{2}:{3})", "contextSequenceId", "opType", "property", "propType")
        SM_TRACE( NamePropOpDefault, 17, Info, "({1})", "contextSequenceId", "opType")
        SM_TRACE( NamePropertyBatch, 18, Info, "({1}={2})", "contextSequenceId", "name", "batch")
        SM_TRACE( ServiceDescription, 19, Info, "{1} ({2}):{3}@{4} ({5}) [{6}] [{7}]/{8}/{9} {10} ", "contextSequenceId", "serviceName", "serviceDnsName", "serviceType", "applicationName", "servicePackageActivationMode",  "packageVersion", "partitionCount", "targetReplicaSetSize", "minReplicaSetSize", "details")
        SM_TRACE( PSDTrace, 20, Info, "{1} : ({2}, {3}) {4} - [{5},{6}] {7}", "contextSequenceId", "service", "partitionScheme", "version", "cuids", "lowRange", "highRange", "partitionNames")
        SM_TRACE( StatefulServiceTableEntryTrace, 21, Info, "CUID={1}, Primary=[{2}], Secondaries=[{3}], Version={4}", "contextSequenceId", "cuid", "primary", "secondaries", "version")
        SM_TRACE( StatelessServiceTableEntryTrace, 22, Info, "CUID={1}, Replicas=[{2}], Version={3}", "contextSequenceId", "cuid", "replicas", "version")
        SM_TRACE( RSPTrace, 23, Info, "RSP(name={1},partition={2},generation={3},locations={4},serviceGroup={5}", "contextSequenceId", "name", "partition", "generation", "locations", "serviceGroup")
        SM_TRACE( ADFTrace, 24, Info, "ADF(name={1},error={2},storeVersion={3})", "contextSequenceId", "name", "error", "storeVersion")
        SM_TRACE( NotifRequestDataTrace, 25, Info, "NotifRequest({1}, pk={2}, previous=({3} CUIDS/error={4};{5}))", "contextSequenceId", "name", "pk", "previousCuids", "previousError", "previousErrorVersion")
        SM_TRACE( ReplicatorSettingsError, 26, Error, "Invalid Replicator Settings - {0}", "message")
        SM_TRACE( ConfigurationParseError, 27, Error, "Invalid Configuration - {0}", "message")
        SM_TRACE( SequenceStreamIdTrace, 28, Info,  "{1}+{2}", "contextSequenceId", "kind", "sourceId")
        SM_TRACE( FailoverUnitId, 29, Info,   "{1}\r\n", "contextSequenceId", "ftId")
        SM_TRACE( ReportFaultRequestMessageBody, 30, Info, "Node = {1} Partition = {2} Replica = {3} FaultType = {4}", "contextSequenceId", "node", "partition", "replica", "faultType")
        SM_TRACE( ReportFaultReplyMessageBody, 31, Info, "Error = {1}, Message = {2}", "contextSequenceId", "error", "message")
        SM_TRACE( ConsistencyUnitDescription, 32, Info, "{0}, {1}{2}", "contextSequenceId", "consistencyUnitId", "partitionKind", "partitionKindDetails")
        SM_TRACE( UnhealthyHealthEvaluationDescription, 33, Info, "\r\n{1} {2}", "contextSequenceId", "description", "unhealthyEvaluations")
        SM_TRACE( HealthEvaluationDescriptionTrace, 34, Info, "{1}", "contextSequenceId", "unhealthyEvaluation")
        SM_TRACE( HealthEvaluationBaseTrace, 35, Info, "\r\n{1}: {2}: [{3}]", "contextSequenceId", "kind", "healthState", "description")
        SM_TRACE( ErrorEventHealthEvaluationTrace, 36, Info, "ErrorEvent: {1}\r\n", "contextSequenceId", "healthEvent")
        SM_TRACE( HealthEventTrace, 37, Info, "SourceId:'{1}' Property:'{2}' state:{3} ttl:{4} sn:{5} sourceUtc:{6} '{7}' lastModifiedUtc:{8} expired:{9} removeWhenExpired:{10} lastOKAt:{11} lastWarningAt:{12} lastErrorAt:{13}", "contextSequenceId", "sourceId", "property", "state", "timeToLive", "sequenceNumber", "sourceUtcTimestamp", "description", "lastModifiedUtcTimestamp", "isExpired", "removeWhenExpired", "lastOkTransitionAt", "lastWarningTransitionAt", "lastErrorTransitionAt")
        SM_TRACE( ServiceUpdateDescription, 40, Info, "flags={1} target={2} min={3} restart={4} ql={5} standby={6} correlations={7} placements={8} metrics={9} policies={10} movecost={11} repartition={12} initdata={13}", "contextSequenceId", "flags", "target", "min", "restart", "ql", "standby", "correlations", "placements", "metrics", "policies", "movecost", "repartition", "initData")
        SM_TRACE( ServiceCorrelationDescription, 41, Info, "name={1} scheme={2}", "contextSequenceId", "name", "scheme")
        SM_TRACE( ServiceLoadMetricDescription, 42, Info, "name={1} weight={2} primary={3} secondary={4}", "contextSequenceId", "name", "weight", "primary", "secondary")
        SM_TRACE( ServicePlacementPolicyDescription, 43, Info, "name={1} type={2}", "contextSequenceId", "name", "type")

        SM_TRACE( RepartitionDescription, 44, Info, "kind={1}", "contextSequenceId", "kind")
        SM_TRACE( NamedRepartitionDescription, 45, Info, "toAdd={1} toRemove={2}", "contextSequenceId", "toAdd", "toRemove")

        SM_TRACE( ServiceHealthTrace, 46, Info, "{1}, {2} events, {3} partitions, stats [{4}].", "contextSequenceId", "events", "healthState", "partitionCount", "stats" )
        SM_TRACE( ServiceHealthWithEvaluationsTrace, 47, Info, "{1}, {2} events, {3} partitions, stats [{4}], {5}", "contextSequenceId", "healthState", "events", "partitionCount", "stats", "unhealthyEvaluation" )
        SM_TRACE( DeployedApplicationHealthTrace, 48, Info, "{1}, {2} events, {3} deployed service packages, stats [{4}].", "contextSequenceId", "healthState", "events", "dspCount", "stats" )
        SM_TRACE( DeployedApplicationHealthWithEvaluationsTrace, 49, Info, "{1}, {2} events, {3} deployed service packages, stats [{4}], {5}", "contextSequenceId", "healthState", "events", "dspCount", "stats", "unhealthyEvaluation" )
        SM_TRACE( ApplicationHealthTrace, 50, Info, "{1}, {2} events, {3} services, {4} deployed apps, stats [{5}].", "contextSequenceId", "healthState", "events", "serviceCount", "daCount", "stats" )
        SM_TRACE( ApplicationHealthWithEvaluationsTrace, 51, Info, "{1}, {2} events, {3} services, {4} deployed apps, stats [{5}], {6}", "contextSequenceId", "healthState", "events", "serviceCount", "daCount", "stats", "unhealthyEvaluation" )
        SM_TRACE( UnhealthyHealthEvaluationWithOneChildDescription, 52, Info, "\r\n{1} {2}", "contextSequenceId", "description", "unhealthyEvaluation" )
        SM_TRACE( UnhealthyHealthEvaluationNoChildDescription, 53, Info, "\r\n{1}", "contextSequenceId", "description" )
        SM_TRACE( ExpiredEventHealthEvaluationTrace, 54, Info, "ExpiredEvent: {1}\r\n", "contextSequenceId", "healthEvent")
        SM_TRACE( UnhealthyEventHealthEvaluationTrace, 55, Info, "UnhealthyEvent, ConsiderWarningAsError {1}: {2}\r\n", "contextSequenceId", "considerWarningAsError", "healthEvent" )
        SM_TRACE( ClusterHealthTrace, 56, Info, "{1}, {2} events, {3} applications, {4} nodes, stats [{5}].", "contextSequenceId", "healthState", "events", "appCount", "nodeCount", "stats" )
        SM_TRACE( ClusterHealthWithEvaluationsTrace, 57, Info, "{1}, {2} events, {3} applications, {4} nodes, stats {5}, UnhealthyEvaluations: {6}", "contextSequenceId", "healthState", "events", "appCount", "nodeCount", "stats", "unhealthyEvaluation" )
        SM_TRACE( EntityHealthBaseTrace, 58, Info, "{1}, {2} events.", "contextSequenceId", "healthState", "events" )
        SM_TRACE( EntityHealthBaseWithEvaluationsTrace, 59, Info, "{1}, {2} events. UnhealthyEvaluations: {3}", "contextSequenceId", "healthState", "events", "unhealthyEvaluation" )
        SM_TRACE( PartitionHealthTrace, 60, Info, "{1}, {2} events, {3} replicas, stats [{4}].", "contextSequenceId", "events", "healthState", "replicaCount", "stats" )
        SM_TRACE( PartitionHealthWithEvaluationsTrace, 61, Info, "{1}, {2} events, {3} replicas, stats [{4}], {5}", "contextSequenceId", "healthState", "events", "replicaCount", "stats", "unhealthyEvaluation" )


        END_STRUCTURED_TRACES

        DECLARE_SM_TRACE( NodeTaskDescription, uint16, std::wstring, Management::ClusterManager::NodeTask::Trace )
        DECLARE_SM_TRACE( NamePropPutOp, uint16, std::wstring, std::wstring, std::wstring, std::wstring)
        DECLARE_SM_TRACE( NamePropGetOp, uint16, std::wstring, std::wstring, bool)
        DECLARE_SM_TRACE( NamePropOp, uint16, std::wstring, std::wstring)
        DECLARE_SM_TRACE( NamePropCheckExistsOp, uint16, std::wstring, std::wstring, bool)
        DECLARE_SM_TRACE( NamePropCheckSequenceOp, uint16, std::wstring, std::wstring, int64)
        DECLARE_SM_TRACE( NamePropCheckValueOp, uint16, std::wstring, std::wstring, std::wstring)
        DECLARE_SM_TRACE( NamePropOpDefault, uint16, std::wstring)
        DECLARE_SM_TRACE( NamePropertyBatch, uint16, std::wstring, std::vector<Naming::NamePropertyOperation>)
        DECLARE_SM_TRACE( ServiceDescription, uint16, std::wstring, std::wstring, std::wstring, std::wstring, ServiceModel::ServicePackageActivationMode::Trace, ServiceModel::ServicePackageVersionInstance, int32, int32, int32, std::wstring)
        DECLARE_SM_TRACE( PSDTrace, uint16, Reliability::ServiceDescription, int, __int64, std::wstring, __int64, __int64, std::wstring)
        DECLARE_SM_TRACE( StatefulServiceTableEntryTrace, uint16, Common::Guid, std::wstring, std::wstring, int64)
        DECLARE_SM_TRACE( StatelessServiceTableEntryTrace, uint16, Common::Guid, std::wstring, int64)
        DECLARE_SM_TRACE( RSPTrace, uint16, std::wstring, std::wstring, Reliability::GenerationNumber, Reliability::ServiceTableEntry, bool)
        DECLARE_SM_TRACE( ADFTrace, uint16, std::wstring, Common::ErrorCode, __int64)
        DECLARE_SM_TRACE( NotifRequestDataTrace, uint16, std::wstring, std::wstring, uint64, std::wstring, __int64)
        DECLARE_SM_TRACE( ReplicatorSettingsError, std::wstring)
        DECLARE_SM_TRACE( ConfigurationParseError, std::wstring)
        DECLARE_SM_TRACE( SequenceStreamIdTrace, uint16, std::wstring, std::wstring)
        DECLARE_SM_TRACE( FailoverUnitId, uint16, Common::Guid)
        DECLARE_SM_TRACE( ReportFaultRequestMessageBody, uint16, std::wstring, Common::Guid, int64, Reliability::FaultType::Trace)
        DECLARE_SM_TRACE( ReportFaultReplyMessageBody, uint16, Common::ErrorCode, std::wstring)
        DECLARE_SM_TRACE( ConsistencyUnitDescription, uint16, Common::Guid, int, std::wstring)
        DECLARE_SM_TRACE( UnhealthyHealthEvaluationDescription, uint16, std::wstring, std::vector<ServiceModel::HealthEvaluation> )
        DECLARE_SM_TRACE( HealthEvaluationDescriptionTrace, uint16, HealthEvaluationBase )
        DECLARE_SM_TRACE( HealthEvaluationBaseTrace, uint16, std::wstring, std::wstring, std::wstring )
        DECLARE_SM_TRACE( ErrorEventHealthEvaluationTrace, uint16, HealthEvent )
        DECLARE_SM_TRACE( HealthEventTrace, uint16, std::wstring, std::wstring, int, Common::TimeSpan, FABRIC_SEQUENCE_NUMBER, Common::DateTime, std::wstring, Common::DateTime, bool, bool, Common::DateTime, Common::DateTime, Common::DateTime )
        DECLARE_SM_TRACE( ServiceUpdateDescription, uint16, int, int, int, Common::TimeSpan, Common::TimeSpan, Common::TimeSpan, std::vector<Reliability::ServiceCorrelationDescription>, std::wstring, std::vector<Reliability::ServiceLoadMetricDescription>, std::vector<ServiceModel::ServicePlacementPolicyDescription>, int, Naming::RepartitionDescription, int )
        DECLARE_SM_TRACE( ServiceCorrelationDescription, uint16, std::wstring, int)
        DECLARE_SM_TRACE( ServiceLoadMetricDescription, uint16, std::wstring, int, int, int)
        DECLARE_SM_TRACE( ServicePlacementPolicyDescription, uint16, std::wstring, int )
        DECLARE_SM_TRACE( RepartitionDescription, uint16, int)
        DECLARE_SM_TRACE( NamedRepartitionDescription, uint16, std::wstring, std::wstring)

        DECLARE_SM_TRACE( ServiceHealthTrace, uint16, std::wstring, uint64, uint64, std::wstring )
        DECLARE_SM_TRACE( ServiceHealthWithEvaluationsTrace, uint16, std::wstring, uint64, uint64, std::wstring, HealthEvaluationBase )
        DECLARE_SM_TRACE( DeployedApplicationHealthTrace, uint16, std::wstring, uint64, uint64, std::wstring )
        DECLARE_SM_TRACE( DeployedApplicationHealthWithEvaluationsTrace, uint16, std::wstring, uint64, uint64, std::wstring, HealthEvaluationBase )
        DECLARE_SM_TRACE( ApplicationHealthTrace, uint16, std::wstring, uint64, uint64, uint64, std::wstring )
        DECLARE_SM_TRACE( ApplicationHealthWithEvaluationsTrace, uint16, std::wstring, uint64, uint64, uint64, std::wstring, HealthEvaluationBase )
        DECLARE_SM_TRACE( UnhealthyHealthEvaluationWithOneChildDescription, uint16, std::wstring, ServiceModel::HealthEvaluationBase)
        DECLARE_SM_TRACE( UnhealthyHealthEvaluationNoChildDescription, uint16, std::wstring )
        DECLARE_SM_TRACE( ExpiredEventHealthEvaluationTrace, uint16, HealthEvent )
        DECLARE_SM_TRACE( UnhealthyEventHealthEvaluationTrace, uint16, bool, HealthEvent )
        DECLARE_SM_TRACE( ClusterHealthTrace, uint16, std::wstring, uint64, uint64, uint64, std::wstring )
        DECLARE_SM_TRACE( ClusterHealthWithEvaluationsTrace, uint16, std::wstring, uint64, uint64, uint64, std::wstring, HealthEvaluationBase )
        DECLARE_SM_TRACE( EntityHealthBaseTrace, uint16, std::wstring, uint64)
        DECLARE_SM_TRACE( EntityHealthBaseWithEvaluationsTrace, uint16, std::wstring, uint64, HealthEvaluationBase )
        DECLARE_SM_TRACE( PartitionHealthTrace, uint16, std::wstring, uint64, uint64, std::wstring )
        DECLARE_SM_TRACE( PartitionHealthWithEvaluationsTrace, uint16, std::wstring, uint64, uint64, std::wstring, HealthEvaluationBase)

        static Common::Global<ServiceModelEventSource> Trace;
    };
}
