// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupStatelessMember, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupStatelessMemberEventSource
    {
    public:
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_0_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_0_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Warning_0_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_1_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_2_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_1_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_3_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_4_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_2_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_5_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_6_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_3_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64, int32);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_7_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_8_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);
        DECLARE_SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_4_StatelessServiceGroupMember, std::wstring, uintptr_t, Common::Guid, int64);

        ServiceGroupStatelessMemberEventSource() :
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_0_StatelessServiceGroupMember, 4, Info, "this={1} partitionId={2} instanceId={3} - Starting IFabricStatelessServiceInstance::BeginOpen call", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_0_StatelessServiceGroupMember, 5, Error, "this={1} partitionId={2} instanceId={3} - IFabricStatelessServiceInstance::BeginOpen call failed with {4}", "id", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Warning_0_StatelessServiceGroupMember, 6, Warning, Common::TraceChannelType::Debug, "this={1} partitionId={2} instanceId={3} - Cleaning up failed service member open", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_1_StatelessServiceGroupMember, 7, Info, "this={1} partitionId={2} instanceId={3} - Starting IFabricStatelessServiceInstance::EndOpen call", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_2_StatelessServiceGroupMember, 8, Info, "this={1} partitionId={2} instanceId={3} - Success opening service instance", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_1_StatelessServiceGroupMember, 9, Error, "this={1} partitionId={2} instanceId={3} - IFabricStatelessServiceInstance::EndOpen call failed with {4}", "id", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_3_StatelessServiceGroupMember, 10, Info, "this={1} partitionId={2} instanceId={3} - Service instance is not opened, ignoring close", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_4_StatelessServiceGroupMember, 11, Info, "this={1} partitionId={2} instanceId={3} - Starting IFabricStatelessServiceInstance::BeginClose call", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_2_StatelessServiceGroupMember, 12, Error, "this={1} partitionId={2} instanceId={3} - IFabricStatelessServiceInstance::BeginClose call failed with {4}", "id", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_5_StatelessServiceGroupMember, 13, Info, "this={1} partitionId={2} instanceId={3} - Service instance is not opened", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_6_StatelessServiceGroupMember, 14, Info, "this={1} partitionId={2} instanceId={3} - Starting IFabricStatelessServiceInstance::EndClose call", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_3_StatelessServiceGroupMember, 15, Error, "this={1} partitionId={2} instanceId={3} - IFabricStatelessServiceInstance::EndClose call failed with {4}", "id", "object", "partitionId", "instanceId", "errorCode"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_7_StatelessServiceGroupMember, 16, Info, "this={1} partitionId={2} instanceId={3} - Service instance is not opened, ignoring abort", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Info_8_StatelessServiceGroupMember, 17, Info, "this={1} partitionId={2} instanceId={3} - IFabricStatelessServiceInstance::Abort call", "id", "object", "partitionId", "instanceId"),
            SERVICEGROUPSTATELESSMEMBER_STRUCTURED_TRACE(Error_4_StatelessServiceGroupMember, 18, Error, Common::TraceChannelType::Debug, "this={1} partitionId={2} instanceId={3} - Simulated stateless service partition unavailable", "id", "object", "partitionId", "instanceId")
        {
        }

        static ServiceGroupStatelessMemberEventSource const & GetEvents();

    };
}
