// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {

#define DECLARE_RM_TRACE(trace_name, ...) \
        DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define RM_TRACE(trace_name, trace_id, trace_level, ...) \
        STRUCTURED_TRACE(trace_name, RepairManager, trace_id, trace_level, __VA_ARGS__),

        class RepairManagerEventSource
        {
            BEGIN_STRUCTURED_TRACES( RepairManagerEventSource )

            // General Request Processing
            //
            RM_TRACE( RequestValidationFailed, 10, Info, "{0} invalid ({1}) message id = {2} reason = {3}", "prId", "action", "messageId", "reason" )
            RM_TRACE( RequestValidationFailed2, 11, Info, "{0} invalid ({1}) message id = {2} reason = {3}", "ractId", "action", "messageId", "reason" )
            RM_TRACE( RequestReceived, 15, Noise, "{0} receive ({1}) message id = {2} timeout = {3}", "ractId", "action", "messageId", "timeout" )
            RM_TRACE( RequestRejected, 20, Info, "{0} reject message id = {1} error = {2}", "ractId", "messageId", "error" )
            RM_TRACE( RequestRetry, 25, Noise, "{0} retry message id = {1} delay = {2} error = {3}", "ractId", "messageId", "delay", "error" )
            RM_TRACE( RequestSucceeded, 30, Noise, "{0} success message id = {1}", "ractId", "messageId" )
            RM_TRACE( RequestFailed, 35, Info, "{0} fail message id = {1} error = {2}", "ractId", "messageId", "error" )

            // Replica Lifecycle
            //
            RM_TRACE( ReplicaOpen, 40, Info, "{0} open node = {1} location = {2} ", "prId", "node", "location" )
            RM_TRACE( ReplicaChangeRole, 45, Info, "{0} change role = {1}", "prId", "role" )
            RM_TRACE( ReplicaClose, 50, Info, "{0} close", "prId" )
            RM_TRACE( ReplicaAbort, 55, Info, "{0} abort", "prId" )

            END_STRUCTURED_TRACES

            // General Request Processing
            //
            DECLARE_RM_TRACE( RequestValidationFailed, Store::PartitionedReplicaId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_RM_TRACE( RequestValidationFailed2, Store::ReplicaActivityId, std::wstring, Transport::MessageId, std::wstring  )
            DECLARE_RM_TRACE( RequestReceived, Store::ReplicaActivityId, std::wstring, Transport::MessageId, Common::TimeSpan )
            DECLARE_RM_TRACE( RequestRejected, Store::ReplicaActivityId, Transport::MessageId, Common::ErrorCode )
            DECLARE_RM_TRACE( RequestRetry, Store::ReplicaActivityId, Transport::MessageId, Common::TimeSpan, Common::ErrorCode )
            DECLARE_RM_TRACE( RequestSucceeded, Store::ReplicaActivityId, Transport::MessageId )
            DECLARE_RM_TRACE( RequestFailed, Store::ReplicaActivityId, Transport::MessageId, Common::ErrorCode )
            
            // Replica Lifecycle
            //
            DECLARE_RM_TRACE( ReplicaOpen, Store::PartitionedReplicaId, Federation::NodeInstance, SystemServices::SystemServiceLocation )
            DECLARE_RM_TRACE( ReplicaChangeRole, Store::PartitionedReplicaId, std::wstring )
            DECLARE_RM_TRACE( ReplicaClose, Store::PartitionedReplicaId )
            DECLARE_RM_TRACE( ReplicaAbort, Store::PartitionedReplicaId )

            static Common::Global<RepairManagerEventSource> Trace;
        };

        typedef RepairManagerEventSource RMEvents; 
    }
}
