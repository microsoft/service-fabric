// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    #define DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define SERVICEGROUPCOMMON_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
        trace_name( \
            Common::TraceTaskCodes::ServiceGroupCommon, \
            trace_id, \
            #trace_name, \
            Common::LogLevel::trace_level, \
            __VA_ARGS__)

    class ServiceGroupCommonEventSource
    {
    public:
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_OperationContext, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_0_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_1_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_2_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_3_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_1_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_2_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_4_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_3_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_4_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_5_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_6_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_5_ServiceGroupFactoryBuilder, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_7_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_6_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_7_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_8_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_9_ServiceGroupFactoryBuilder, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_0_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Warning_0_ServiceGroupCommon, std::wstring, uintptr_t, std::wstring);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_1_ServiceGroupCommon, std::wstring, uintptr_t, std::wstring, uint32, Common::Guid);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_1_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_2_ServiceGroupCommon, std::wstring, uintptr_t, std::wstring, uint32, Common::Guid);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_2_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_3_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_4_ServiceGroupCommon, std::wstring, uintptr_t);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_3_ServiceGroupCommon, std::wstring, uintptr_t, std::wstring, uint32);
        DECLARE_SERVICEGROUPCOMMON_STRUCTURED_TRACE(ReplicaAlreadyClosed, std::wstring, uintptr_t, int64);
        
        ServiceGroupCommonEventSource() :
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_OperationContext, 4, Info, "this={0} - Cancel not supported", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_ServiceGroupFactoryBuilder, 5, Info, "this={0} - Adding stateless service factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_0_ServiceGroupFactoryBuilder, 6, Error, Common::TraceChannelType::Debug, "this={0} - Member service type name is invalid", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_1_ServiceGroupFactoryBuilder, 7, Error, Common::TraceChannelType::Debug, "this={0} - Service type {1} already registered as stateful (mixed mode not supported)", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_2_ServiceGroupFactoryBuilder, 8, Error, "this={0} - Could not store new factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_3_ServiceGroupFactoryBuilder, 9, Error, Common::TraceChannelType::Debug, "this={0} - Factory for service type {1} already added", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_1_ServiceGroupFactoryBuilder, 10, Info, "this={0} - Added stateless service factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_2_ServiceGroupFactoryBuilder, 11, Info, "this={0} - Adding stateful service factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_4_ServiceGroupFactoryBuilder, 12, Error, Common::TraceChannelType::Debug, "this={0} - Service type {1} already registered as stateless (mixed mode not supported)", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_3_ServiceGroupFactoryBuilder, 13, Info, "this={0} - Added stateful service factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_4_ServiceGroupFactoryBuilder, 14, Info, "this={0} - Removing factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_5_ServiceGroupFactoryBuilder, 15, Info, "this={0} - Removing stateless factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_6_ServiceGroupFactoryBuilder, 16, Info, "this={0} - Removing stateful factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_5_ServiceGroupFactoryBuilder, 17, Error, Common::TraceChannelType::Debug, "this={0} - Could not remove factory for service type {1}", "object", "memberServiceType"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_7_ServiceGroupFactoryBuilder, 18, Info, "this={0} - Building service group factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_6_ServiceGroupFactoryBuilder, 19, Error, "this={0} - Could not create stateful class factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_7_ServiceGroupFactoryBuilder, 20, Error, "this={0} - Could not initialize stateful class factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_8_ServiceGroupFactoryBuilder, 21, Error, "this={0} - Could not create stateless class factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_9_ServiceGroupFactoryBuilder, 22, Error, "this={0} - Could not initialize stateless class factory", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_0_ServiceGroupCommon, 23, Info, "this={1} - Update load metrics", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_0_ServiceGroupCommon, 24, Error, Common::TraceChannelType::Debug, "this={1} - Metric name is invalid", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Warning_0_ServiceGroupCommon, 25, Warning, Common::TraceChannelType::Debug, "this={1} - Metric {2} reported more than once", "id", "object", "name"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_1_ServiceGroupCommon, 26, Info, "this={1} - Metric ({2},{3}) reported by service member {4}", "id", "object", "name", "metricValue", "serviceReporting"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_1_ServiceGroupCommon, 27, Error, "this={1} - Could not store initial metrics", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_2_ServiceGroupCommon, 28, Info, "this={1} - Metric ({2},{3}) reported by service member {4} has not changed", "id", "object", "name", "metricValue", "serviceReporting"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_2_ServiceGroupCommon, 29, Error, "this={1} - Could not store new metrics", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_3_ServiceGroupCommon, 30, Error, Common::TraceChannelType::Debug, "this={1} Overflow metric", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Error_4_ServiceGroupCommon, 31, Error, "this={1} - Could not compute metrics", "id", "object"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(Info_3_ServiceGroupCommon, 32, Info, "this={1} - Updated metric {2} value is {3}", "id", "object", "name", "updateMetricValue"),
            SERVICEGROUPCOMMON_STRUCTURED_TRACE(ReplicaAlreadyClosed, 33, Warning, Common::TraceChannelType::Debug, "this={1} replicaId={2} - Replica already closed", "id", "object", "replicaId")
        {
        }

        static ServiceGroupCommonEventSource const & GetEvents();

    };
}
