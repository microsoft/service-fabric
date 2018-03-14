// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
#define HM_COMMON_TRACE(trace_name, trace_id, trace_level, ...) \
        STRUCTURED_TRACE(trace_name, HealthManagerCommon, trace_id, trace_level, __VA_ARGS__)

        class HealthManagerCommonEventSource
        {
        public:
        HealthManagerCommonEventSource()
            : HM_COMMON_TRACE( ReportRequestContextTrace, 10, Info, "{1}:{2}", "contextSequenceId", "raId", "report" )
            , HM_COMMON_TRACE( QueryRequestContextTrace, 11, Info, "{1}:{2}({3})", "contextSequenceId", "raId", "queryKind", "entityId" )
            , HM_COMMON_TRACE( HealthEventStoreDataTrace, 12, Info, "{1}({2},{3},Generated@{4}, Updated@{5}, ttl={6}, reportLSN={7}, desc={8}, transient={9}.{10})", "contextSequenceId", "type", "key", "state", "sourceUtc", "lastModifiedUtc", "ttl", "sn", "desc", "transient", "history" )
            , HM_COMMON_TRACE( NodeAttributesStoreDataTrace, 13, Info, "{1}({2}:name='{3}',addr='{4}',UD='{5}',FD='{6}',instanceId={7},{8})", "contextSequenceId", "type", "key", "name", "ip", "ud", "fd", "instanceId", "state" )
            , HM_COMMON_TRACE( ReplicaAttributesStoreDataTrace, 14, Info, "{1}({2}:nodeId='{3}',instanceId={4},nodeInstance={5},{6})", "contextSequenceId", "type", "key", "nodeId", "instanceId", "nodeInstance", "state" )
            , HM_COMMON_TRACE( PartitionAttributesStoreDataTrace, 15, Info, "{1}({2}:serviceName='{3}',{4})", "contextSequenceId", "type", "key", "serviceName", "state" )
            , HM_COMMON_TRACE( JobItemTrace, 17, Info, "{1}({2},id={3},{4},JI lsn={5}, priority={6})", "contextSequenceId", "type", "raId", "entityId", "state", "jiLsn", "priority" )
            , HM_COMMON_TRACE( SequenceStreamStoreDataTrace, 18, Info, "{1}({2}:{3}=[0,{4})) invalidate: {5}@{6}, highest: {7}", "contextSequenceId", "type", "key", "instance", "upToLsn", "invalidateLsn", "invalidateTime", "highestLsn" )
            , HM_COMMON_TRACE( SequenceStreamRequestContextTrace, 19, Info, "{1}:SequenceStreamContext(id={2},{3}+{4}:{5} [{6}, {7}), invalidate:{8})", "contextSequenceId", "raId", "requestId", "sourceId", "entityType", "instance", "fromLsn", "upToLsn", "invalidateLsn")
            , HM_COMMON_TRACE( QueryRequestListContextTrace, 23, Info, "{1}:QueryListContext({2},filter state={3})", "contextSequenceId", "raId", "type", "filterState" )
            , HM_COMMON_TRACE( ServiceAttributesStoreDataTrace, 24, Info, "{1}({2}:instanceId={3},appName={4},serviceTypeName={5},{6})", "contextSequenceId", "type", "key", "instanceId", "appName", "serviceTypeName", "state" )
            , HM_COMMON_TRACE( ApplicationAttributesStoreDataTrace, 25, Info, "{1}({2}:instance={3},policy={4},appType={5},definitionKind={6},state={7})", "contextSequenceId", "type", "key", "instance", "policy", "appType", "definitionKind", "state" )
            , HM_COMMON_TRACE( DeployedApplicationAttributesStoreDataTrace, 27, Info, "{1}({2}:nodeName='{3}',instanceId={4},nodeInstanceId={5},{6})", "contextSequenceId", "type", "key", "name", "instanceId", "nodeInstanceId", "state" )
            , HM_COMMON_TRACE( DeployedServicePackageAttributesStoreDataTrace, 28, Info, "{1}({2}:nodeName='{3}',instanceId={4},nodeInstanceId={5},{6})", "contextSequenceId", "type", "key", "name", "instanceId", "nodeInstanceId", "state" )
            , HM_COMMON_TRACE( ClusterAttributesStoreDataTrace, 29, Info, "{1} {2} {3} {4} {5}", "contextSequenceId", "key", "policy", "stateFlags", "upgradePolicy", "applicationHealthPolicies" )
        {
        }

        // Declarations

        DECLARE_STRUCTURED_TRACE( ReportRequestContextTrace, uint16, std::wstring, ServiceModel::HealthReport );
        DECLARE_STRUCTURED_TRACE( QueryRequestContextTrace, uint16, Common::ActivityId, RequestContextKind::Trace, std::wstring );
        DECLARE_STRUCTURED_TRACE( HealthEventStoreDataTrace, uint16, std::wstring, std::wstring, std::wstring, Common::DateTime, Common::DateTime, Common::TimeSpan, FABRIC_SEQUENCE_NUMBER, std::wstring, bool, std::wstring );
        DECLARE_STRUCTURED_TRACE( NodeAttributesStoreDataTrace, uint16, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, FABRIC_NODE_INSTANCE_ID, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( ReplicaAttributesStoreDataTrace, uint16, std::wstring, std::wstring, Common::LargeInteger, FABRIC_INSTANCE_ID, FABRIC_NODE_INSTANCE_ID, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( PartitionAttributesStoreDataTrace, uint16, std::wstring, std::wstring, std::wstring, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( JobItemTrace, uint16, std::wstring, std::wstring, std::wstring, EntityJobItemState::Trace, uint64, ServiceModel::Priority::Trace );
        DECLARE_STRUCTURED_TRACE( SequenceStreamStoreDataTrace, uint16, std::wstring, std::wstring, FABRIC_INSTANCE_ID, uint64, uint64, Common::DateTime, uint64);
        DECLARE_STRUCTURED_TRACE( SequenceStreamRequestContextTrace, uint16, Store::ReplicaActivityId, uint64, std::wstring, std::wstring, FABRIC_INSTANCE_ID, FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER );
        DECLARE_STRUCTURED_TRACE( QueryRequestListContextTrace, uint16, std::wstring, HealthEntityKind::Trace, std::wstring );
        DECLARE_STRUCTURED_TRACE( ServiceAttributesStoreDataTrace, uint16, std::wstring, std::wstring, FABRIC_INSTANCE_ID, std::wstring, std::wstring, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( ApplicationAttributesStoreDataTrace, uint16, std::wstring, std::wstring, FABRIC_INSTANCE_ID, std::wstring, std::wstring, std::wstring, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( DeployedApplicationAttributesStoreDataTrace, uint16, std::wstring, std::wstring, std::wstring, FABRIC_INSTANCE_ID, FABRIC_NODE_INSTANCE_ID, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( DeployedServicePackageAttributesStoreDataTrace, uint16, std::wstring, std::wstring, std::wstring, FABRIC_INSTANCE_ID, FABRIC_NODE_INSTANCE_ID, EntityStateFlags );
        DECLARE_STRUCTURED_TRACE( ClusterAttributesStoreDataTrace, uint16, std::wstring, std::wstring, EntityStateFlags, std::wstring, std::wstring );

        static Common::Global<HealthManagerCommonEventSource> Trace;
        };

        typedef HealthManagerCommonEventSource HMCommonEvents;
    }
}
