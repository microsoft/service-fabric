// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupStateful, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupStatefulEventSource
    {
    public:
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_9_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_10_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_11_StatefulCompositeOpenUndoCloseOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeDataLossUndoOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeDataLossUndoOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeDataLossUndoOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeDataLossUndoOperationAsync, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeDataLossUndoOperationAsync, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_UpdateEpochOperation, uintptr_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_UpdateEpochOperation, uintptr_t, uint32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_UpdateEpochOperation, uintptr_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupPackage, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupPackage, uintptr_t, Common::Guid, uint32, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupPackage, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupPackage, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_6_StatefulServiceGroupPackage, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_7_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_9_StatefulServiceGroupPackage, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupFactory, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupFactory, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupFactory, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupFactory, uintptr_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroup, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroup, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroup, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroup, uintptr_t, int64, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroup, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroup, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroup, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroup, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_6_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportFault, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_7_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_9_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_10_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_11_StatefulServiceGroup, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_12_StatefulServiceGroup, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_13_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_14_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupClose, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupClose, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroupClose, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_15_StatefulServiceGroup, uintptr_t, int64, uint32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, int32, int32, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, int32, int32, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, int32, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, std::wstring, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupUpdateEpoch, uintptr_t, int64, std::wstring, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOpen, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOpen, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, std::wstring, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupChangeRole, std::wstring, uintptr_t, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupClose, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupAbort, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_16_StatefulServiceGroup, uintptr_t, int64, uint32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_17_StatefulServiceGroup, uintptr_t, int64, uint32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_18_StatefulServiceGroup, uintptr_t, int64, uint32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64, int64, int64, int64, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroup, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_19_StatefulServiceGroup, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulServiceGroup, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOnDataLoss, std::wstring, uintptr_t, int64, BOOLEAN);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_9_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, size_t);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupUpdateEpoch, std::wstring, uintptr_t, int64, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_10_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_11_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_12_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_13_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupActivation, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupActivation, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupActivation, std::wstring, uintptr_t, int64, int32, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupActivation, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_14_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_15_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroupPackage, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeUndoProgressOperation, uintptr_t, Common::Guid, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeUndoProgressOperation, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeUndoProgressOperation, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeUndoProgressOperation, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulCompositeUndoProgressOperation, uintptr_t, Common::Guid, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupMember, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupMember, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulReportFault, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulReportFault, std::wstring, uintptr_t, int64, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_2_StatefulReportFault, std::wstring, uintptr_t, int64, std::wstring, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_3_StatefulReportFault, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupAbort, std::wstring, uintptr_t, int64, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_4_StatefulReportFault, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupAbort, std::wstring, uintptr_t, int64, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_5_StatefulReportFault, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, BOOLEAN, BOOLEAN);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_2_StatefulServiceGroupAbort, std::wstring, uintptr_t, Common::Guid, int64, std::wstring, BOOLEAN, BOOLEAN);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid, std::wstring, int32);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulCompositeRollback, Common::StringLiteral, uintptr_t, Common::Guid, std::wstring);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportMoveCost, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportReplicaHealth, std::wstring, uintptr_t, int64);
        DECLARE_SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportPartitionHealth, std::wstring, uintptr_t, int64);
        
        ServiceGroupStatefulEventSource() :
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeOpenUndoCloseOperationAsync, 4, Info, "this={0} partitionId={1} - Starting open", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeOpenUndoCloseOperationAsync, 5, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting open failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeOpenUndoCloseOperationAsync, 6, Info, "this={0} partitionId={1} - Starting close", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeOpenUndoCloseOperationAsync, 7, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting close failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeOpenUndoCloseOperationAsync, 8, Info, "this={0} partitionId={1} - Open completed with error code {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulCompositeOpenUndoCloseOperationAsync, 9, Info, "this={0} partitionId={1} - Invoking callback", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulCompositeOpenUndoCloseOperationAsync, 10, Info, "this={0} partitionId={1} - Finishing open", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulCompositeOpenUndoCloseOperationAsync, 11, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing open failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulCompositeOpenUndoCloseOperationAsync, 12, Info, "this={0} partitionId={1} - Finishing open success", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulCompositeOpenUndoCloseOperationAsync, 13, Info, "this={0} partitionId={1} - Starting undo progress", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulCompositeOpenUndoCloseOperationAsync, 14, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting undo progress failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulCompositeOpenUndoCloseOperationAsync, 15, Info, "this={0} partitionId={1} - No undo progress required", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulCompositeOpenUndoCloseOperationAsync, 16, Info, "this={0} partitionId={1} - Finishing undo progress", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulCompositeOpenUndoCloseOperationAsync, 17, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing undo progress failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_9_StatefulCompositeOpenUndoCloseOperationAsync, 18, Info, "this={0} partitionId={1} - Finishing undo progress success", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_10_StatefulCompositeOpenUndoCloseOperationAsync, 19, Info, "this={0} partitionId={1} - Finishing close", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulCompositeOpenUndoCloseOperationAsync, 20, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing close failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_11_StatefulCompositeOpenUndoCloseOperationAsync, 21, Info, "this={0} partitionId={1} - Finishing close success", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeDataLossUndoOperationAsync, 22, Info, "this={0} partitionId={1} - Starting data loss", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeDataLossUndoOperationAsync, 23, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Starting data loss failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeDataLossUndoOperationAsync, 24, Info, "this={0} partitionId={1} - Finishing data loss", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeDataLossUndoOperationAsync, 25, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Finishing data loss failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeDataLossUndoOperationAsync, 26, Info, "this={0} partitionId={1} - Finishing data loss success", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_UpdateEpochOperation, 27, Info, "this={0} - Acknowledge", "object"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_UpdateEpochOperation, 28, Error, "this={0} - CreateEvent failed with {1}", "object", "win32LastError"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_UpdateEpochOperation, 29, Error, Common::TraceChannelType::Debug, "this={0} - Cancel", "object"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupPackage, 30, Warning, "this={0} partitionId={1} - {2} do not contain {3}. Replicator may fail to initialize.", "object", "partitionId", "sectionName", "propertyName"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupPackage, 31, Error, "Reading security settings failed with {0}", "error"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupPackage, 32, Info, "this={0} partitionId={1} - Replicator settings found. FABRIC_REPLICATOR_SETTINGS created with Flags = {2}, {3}", "object", "partitionId", "flags", "settings"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupPackage, 33, Info, "this={0} partitionId={1} - Package contains no replicator settings. Using default values instead.", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupPackage, 34, Error, "this={0} partitionId={1} - Reading parameter {2} failed with {3}", "object", "partitionId", "name", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupPackage, 35, Error, "this={0} partitionId={1} - Reading parameter {2} failed. Not a valid integer.", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupPackage, 36, Error, "this={0} partitionId={1} - Reading parameter {2} failed. Value out of range.", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupPackage, 37, Error, "this={0} partitionId={1} - Reading parameter {2} failed. Invalid value. Must be true or false.", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupPackage, 38, Error, "this={0} partitionId={1} - TcpTransportUtility::GetLocalFqdn failed with error {2}", "object", "partitionId", "error"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_6_StatefulServiceGroupPackage, 39, Error, "this={0} partitionId={1} - Service manifest does not contain endpoints", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_7_StatefulServiceGroupPackage, 40, Error, "this={0} partitionId={1} - Invalid protocol for replicator endpoint, must be {2}", "object", "partitionId", "value"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroupPackage, 41, Error, "this={0} partitionId={1} - Invalid type for replicator endpoint, must be {2}", "object", "partitionId", "value"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_9_StatefulServiceGroupPackage, 42, Error, "this={0} partitionId={1} - Service manifest does not contain endpoint resource with name {2}", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupFactory, 43, Error, "this={0} - Could not allocate stateful service group", "object"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupFactory, 44, Error, "this={0} - Replica {1} deserialization failed with {2}", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupFactory, 45, Error, "this={0} - Replica {1} initialization failed with {2}", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupFactory, 46, Info, "this={0} - Replica {1} initialization succeeded", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupFactory, 47, Error, Common::TraceChannelType::Debug, "this={0} - No available service factories", "object"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupFactory, 48, Error, "this={0} - Could not store service factories", "object"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroup, 49, Info, "this={0} replicaId={1} - Starting initialization", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroup, 50, Error, "this={0} replicaId={1} - Could not allocate stateful service member for service type {2}", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroup, 51, Error, "this={0} replicaId={1} - Could not find registered stateful service factory for service type {2}", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroup, 52, Error, "this={0} replicaId={1} - Stateful service member {2} initialization failed with {3}", "object", "replicaId", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroup, 53, Info, "this={0} replicaId={1} - Stateful service member {2} initialization succeeded", "object", "replicaId", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroup, 54, Error, "this={0} replicaId={1} - Could not store stateful service member {2}", "object", "replicaId", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroup, 55, Info, "this={0} replicaId={1} - Stateful service member {2} stored successfully", "object", "replicaId", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroup, 56, Error, "this={0} replicaId={1} - Initialization failed with {2}", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroup, 57, Info, "this={0} replicaId={1} - Received activation context", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroup, 58, Info, "this={0} replicaId={1} - Initialization succeeded", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroup, 59, Info, "this={1} replicaId={2} - Uninitialization", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroup, 60, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for GetPartitionInfo", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_6_StatefulServiceGroup, 61, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for ReportLoad", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportFault, 62, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for ReportFault", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_7_StatefulServiceGroup, 63, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for GetReadStatus", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroup, 64, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for GetWriteStatus", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_9_StatefulServiceGroup, 65, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System replicator unavailable", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroup, 66, Info, "this={1} replicaId={2} - Returning simulated state replicator and system fabric replicator", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_10_StatefulServiceGroup, 67, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Service member name is invalid", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_11_StatefulServiceGroup, 68, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Service member name {3} is not a valid URI", "id", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_12_StatefulServiceGroup, 69, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Service member {3} not found", "id", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_13_StatefulServiceGroup, 70, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Could not resolve service member", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_14_StatefulServiceGroup, 71, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOpen, 72, Info, "this={1} replicaId={2} - Starting open", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOpen, 73, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Begin open failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOpen, 74, Info, "this={1} replicaId={2} - Begin open success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupClose, 75, Info, "this={1} replicaId={2} - Starting close in role {3}", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupClose, 76, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Close failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_8_StatefulServiceGroupClose, 77, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Begin close failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroupClose, 78, Info, "this={1} replicaId={2} - Begin close success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupChangeRole, 79, Info, "this={1} replicaId={2} - Starting change role from current role {3} to new role {4}", "id", "object", "replicaId", "currentReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_15_StatefulServiceGroup, 80, Error, "this={0} replicaId={1} - CreateEvent atomic group replication failed with {2}", "object", "replicaId", "win32LastError"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupChangeRole, 81, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Failed change role from {3} to {4} with {5}", "id", "object", "replicaId", "currentReplicaRole", "newReplicaRole", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupChangeRole, 82, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - StartOperationStreams failed with {3} during change role from {4} to {5}", "id", "object", "replicaId", "errorCode", "currentReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupChangeRole, 83, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - UpdateOperationStreams failed with {3} during change role from {4} to {5}", "id", "object", "replicaId", "errorCode", "currentReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupChangeRole, 84, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Changing role from role {3} to {4} failed with {5}", "id", "object", "replicaId", "currentReplicaRole", "newReplicaRole", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupChangeRole, 85, Info, "this={1} replicaId={2} - Begin change role from {3} to {4} success", "id", "object", "replicaId", "oldReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupUpdateEpoch, 86, Info, "this={0} replicaId={1} - Current replica role is {2}, previous epoch sequence number is {3}", "object", "replicaId", "currentReplicaRole", "previousEpochLastSequenceNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupUpdateEpoch, 87, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - GetWriteStatus failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupUpdateEpoch, 88, Error, "this={1} replicaId={2} - UpdateEpoch while FABRIC_REPLICA_ROLE_PRIMARY with write-status FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupUpdateEpoch, 89, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Drain failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupUpdateEpoch, 90, Info, "this={1} replicaId={2} - Called with existent epoch (DataLossNumber={3}, ConfigurationNumber={4})", "id", "object", "replicaId", "dataLossNumber", "configurationNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupUpdateEpoch, 91, Info, "this={1} replicaId={2} - Rolling back in-flight atomic groups with previous epoch sequence number {3}", "id", "object", "replicaId", "previousEpochLastSequenceNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupUpdateEpoch, 92, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Begin update epoch failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupUpdateEpoch, 93, Info, "this={1} replicaId={2} - Begin update epoch success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupOnDataLoss, 94, Info, "this={1} replicaId={2} - Starting on data loss in role {3}", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupOnDataLoss, 95, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Data loss failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOnDataLoss, 96, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Begin data loss failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupOnDataLoss, 97, Info, "this={1} replicaId={2} - Begin data loss success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupOpen, 98, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - IFabricStatefulServicePartition::GetPartitionInfo failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOpen, 99, Error, "this={0} replicaId={1} - Could not store service instance name", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_3_StatefulServiceGroupOpen, 100, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - IFabricStatefulServicePartition::CreateReplicator failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupOpen, 101, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Cleaning up failed open", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOpen, 102, Info, "this={1} replicaId={2} - Completing open", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupOpen, 103, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Open failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOpen, 104, Info, "this={1} replicaId={2} - End open success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOpen, 105, Info, "this={1} replicaId={2} - Returning fabric replicator", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupChangeRole, 106, Info, "this={1} replicaId={2} - Completing change role", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_4_StatefulServiceGroupChangeRole, 107, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Changing role from {3} to {4} failed with {5}", "id", "object", "replicaId", "currentReplicaRole", "newReplicaRole", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupChangeRole, 108, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Failed to retrieve composite service endpoint with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupChangeRole, 109, Info, "this={1} replicaId={2} - End change role from {3} to {4} success", "id", "object", "replicaId", "currentReplicaRole", "newReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupClose, 110, Info, "this={1} replicaId={2} - Completing close", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupClose, 111, Info, "this={1} replicaId={2} - End close success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupAbort, 112, Info, "this={1} replicaId={2} - Abort in role {3}", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_16_StatefulServiceGroup, 113, Error, "this={0} replicaId={1} - CreateEvent copy failed with {2}", "object", "replicaId", "win32LastError"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_17_StatefulServiceGroup, 114, Error, "this={0} replicaId={1} - CreateEvent replication failed with {2}", "object", "replicaId", "win32LastError"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_18_StatefulServiceGroup, 115, Error, "this={0} replicaId={1} - CreateEvent atomic group copy state failed with {2}", "object", "replicaId", "win32LastError"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroupUpdateEpoch, 116, Info, "this={1} replicaId={2} - Completing update epoch", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_5_StatefulServiceGroupUpdateEpoch, 117, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Update epoch failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroupUpdateEpoch, 118, Info, 
                    "this={1} replicaId={2} - Updating epoch from "
                    "(DataLossNumber={3}, ConfigurationNumber={4}, PreviousEpochLastSequenceNumber={5})"
                    " to "
                    "(DataLossNumber={6}, ConfigurationNumber={7}, PreviousEpochLastSequenceNumber={8})", 
                    "id", "object", "replicaId", 
                    "fromDataLossNumber", "fromConfigurationNumber", "fromPreviousEpochLsn", 
                    "toDataLossNumber", "toConfigurationNumber", "toPreviousEpochLastSequenceNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroup, 119, Info, "this={1} replicaId={2} - Iterating IFabricStateProvider::GetLastCommittedSequenceNumber calls", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_19_StatefulServiceGroup, 120, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - GetLastCommittedSequenceNumber failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulServiceGroup, 121, Info, "this={1} replicaId={2} - GetLastCommittedSequenceNumber succeed with max lastCommitedSequenceNumber={3}", "id", "object", "replicaId", "maxSequenceNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupOnDataLoss, 122, Info, "this={1} replicaId={2} - Completing on data loss", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulServiceGroupOnDataLoss, 123, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - On data loss failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupOnDataLoss, 124, Info, "this={1} replicaId={2} - End on data loss success", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupOnDataLoss, 125, Info, "this={1} replicaId={2} - Returning state changed {3}", "id", "object", "replicaId", "isStateChanged"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_7_StatefulServiceGroupUpdateEpoch, 126, Info, "this={1} replicaId={2} - Keeping atomic group {3}", "id", "object", "replicaId", "member"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_8_StatefulServiceGroupUpdateEpoch, 127, Info, "this={1} replicaId={2} - Removing atomic group {3}", "id", "object", "replicaId", "member"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_9_StatefulServiceGroupUpdateEpoch, 128, Info, "this={1} replicaId={2} - Remaining atomic groups in-flight {3}", "id", "object", "replicaId", "count"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupUpdateEpoch, 129, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Removing all remaining atomic groups due to data loss", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupUpdateEpoch, 130, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Removing atomic group {3} due to data loss", "id", "object", "replicaId", "member"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupPackage, 131, Info, "this={1} replicaId={2} - Begin package processing", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulServiceGroupPackage, 132, Info, "this={1} replicaId={2} - Configuration package {3} does not contain replicator settings", "id", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_10_StatefulServiceGroupPackage, 133, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - error comparing package names", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_11_StatefulServiceGroupPackage, 134, Error, "this={1} replicaId={2} - Failed reading replicator settings from config package with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_12_StatefulServiceGroupPackage, 135, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful replicator unavailable", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_13_StatefulServiceGroupPackage, 136, Error, "this={1} replicaId={2} - Failed updating replicator settings from config package with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulServiceGroupPackage, 137, Info, "this={1} replicaId={2} - Update replicator settings completed successfully", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupActivation, 138, Info, "this={1} replicaId={2} - Service is not hosted. Use default values for replicator settings.", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulServiceGroupActivation, 139, Info, "this={1} replicaId={2} - ConfigurationPackage {3} not found, use default values for replicator settings", "id", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupActivation, 140, Error, "this={1} replicaId={2} - ReplicatorSettings::ReadFromConfigurationPackage failed with {3}, package={4}", "id", "object", "replicaId", "errorCode", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupActivation, 141, Info, "this={1} replicaId={2} - ReplicatorSettings::ReadFromConfigurationPackage succeeded package={3}", "id", "object", "replicaId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_14_StatefulServiceGroupPackage, 142, Error, "this={1} replicaId={2} - Register configuration package change handler failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulServiceGroupPackage, 143, Info, "this={1} replicaId={2} - Register configuration package change handler succeeded", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_15_StatefulServiceGroupPackage, 144, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Unregister configuration package change handler failed with {3}", "id", "object", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_6_StatefulServiceGroupPackage, 145, Info, "this={1} replicaId={2} - Unregister configuration package change handler succeeded", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeUndoProgressOperation, 146, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - GetLastCommittedSequenceNumber for {2} failed with {3}", "object", "partitionId", "name", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeUndoProgressOperation, 147, Info, "this={0} partitionId={1} - Not enough changes to start operation", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeUndoProgressOperation, 148, Info, "this={0} partitionId={1} - Starting undo progress from SequenceNumber={2}", "object", "partitionId", "minSequenceNumber"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeUndoProgressOperation, 149, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} - Failed with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_2_StatefulCompositeUndoProgressOperation, 150, Error, "this={0} partitionId={1} - Failed to store stateful service member with {2}", "object", "partitionId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulServiceGroupMember, 151, Info, "this={0} partitionId={1} replicaId={2} - Starting initialization", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulServiceGroupMember, 152, Error, Common::TraceChannelType::Debug, "this={0} partitionId={1} replicaId={2} - SizeTToULong conversion failed with {3}", "object", "partitionId", "replicaId", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulServiceGroupMember, 153, Error, "this={0} partitionId={1} replicaId={2} - Could not store service member name", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulServiceGroupMember, 154, Info, "this={0} partitionId={1} replicaId={2} - Uninitialization", "object", "partitionId", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulReportFault, 155, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Reporting fault to the system partition failed with {3}", "id", "object", "replicaId", "error"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulReportFault, 156, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Report fault {3} is now in progress", "id", "object", "replicaId", "faultType"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_2_StatefulReportFault, 157, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Current replica role is {3} service member {4} invoking", "id", "object", "replicaId", "currentReplicaRole", "serviceName"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_3_StatefulReportFault, 158, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Current replica role is {3} service group invoking", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_0_StatefulServiceGroupAbort, 159, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Current replica role is {3} service group invoking", "id", "object", "replicaId", "currentReplicaRole"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_4_StatefulReportFault, 160, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Completed fault replica {3}", "id", "object", "replicaId", "faultingReplicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_1_StatefulServiceGroupAbort, 161, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Completed abort replica {3}", "id", "object", "replicaId", "faultingReplicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_5_StatefulReportFault, 162, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Faulting in role {4}. Force draining operation stream with null dispatch flag on copy={5} and on replication={6}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "waitForNullDispatchOnCopyStream", "waitForNullDispatchOnReplicationStream"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Warning_2_StatefulServiceGroupAbort, 163, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} replicaId={3} - Aborting in role {4} with null dispatch flag on copy={5} and on replication={6}", "id", "object", "partitionId", "replicaId", "currentReplicaRole", "waitForNullDispatchOnCopyStream", "waitForNullDispatchOnReplicationStream"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_0_StatefulCompositeRollback, 164, Info, "this={1} partitionId={2} - Starting rollback", "id", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_1_StatefulCompositeRollback, 165, Info, "this={1} partitionId={2} - Starting {3}", "id", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulCompositeRollback, 166, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Starting {3} failed with {4}", "id", "object", "partitionId", "name", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_2_StatefulCompositeRollback, 167, Info, "this={1} partitionId={2} - Finishing rollback", "id", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_3_StatefulCompositeRollback, 168, Info, "this={1} partitionId={2} - Finishing rollback success", "id", "object", "partitionId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_4_StatefulCompositeRollback, 169, Info, "this={1} partitionId={2} - Finishing {3}", "id", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_1_StatefulCompositeRollback, 170, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} - Finishing {3} failed with {4}", "id", "object", "partitionId", "name", "errorCode"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Info_5_StatefulCompositeRollback, 171, Info, "this={1} partitionId={2} - Finishing {3} success", "id", "object", "partitionId", "name"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportMoveCost, 172, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} - System stateful service partition unavailable for ReportMoveCost", "id", "object", "replicaId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportReplicaHealth, 173, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} -System stateful service partition unavailable for ReplicaHealth", "id", "object", "instanceId"),
            SERVICEGROUPSTATEFUL_STRUCTURED_TRACE(Error_0_StatefulReportPartitionHealth, 174, Error, Common::TraceChannelType::Debug, "this={1} replicaId={2} -System stateful service partition unavailable for ReportPartitionHealth", "id", "object", "instanceId")
        {
        }

        static ServiceGroupStatefulEventSource const & GetEvents();

    };
}
