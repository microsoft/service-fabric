// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreServiceEventSource
    { 
    public:
        Common::TraceEventWriter<std::wstring> StoreOpenComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64> AuthorityOwnerInnerCreateNameRequestSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerInnerCreateNameReplyReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateNameRequestReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateNameRequestProcessComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64> AuthorityOwnerInnerDeleteNameRequestSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerInnerDeleteNameReplyReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteNameRequestReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteNameRequestProcessComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64> AuthorityOwnerInnerCreateServiceRequestToNameOwnerSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerInnerCreateServiceReplyFromNameOwnerReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateServiceToFMSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateServiceReplyFromFMReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateServiceRequestReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerCreateServiceRequestProcessComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring, std::wstring, uint64> AuthorityOwnerInnerDeleteServiceRequestToNameOwnerSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerInnerDeleteServiceReplyFromNameOwnerReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteServiceToFMSendComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteServiceReplyFromFMReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteServiceRequestReceiveComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerInnerDeleteServiceRequestProcessComplete;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> StartupRepairStarted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerHierarchyRepairStarted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerServiceRepairStarted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerServiceRepairStarted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> StartupRepairCompleted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerHierarchyRepairCompleted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> AuthorityOwnerServiceRepairCompleted;
        Common::TraceEventWriter<std::wstring, std::wstring, uint64, std::wstring> NameOwnerServiceRepairCompleted;

        StoreServiceEventSource()
            : StoreOpenComplete(Common::TraceTaskCodes::NamingStoreService, 4, "StoreOpenComplete", Common::LogLevel::Info, "{0}: Store service has opened", "storeTraceId"),
            AuthorityOwnerInnerCreateNameRequestSendComplete(Common::TraceTaskCodes::NamingStoreService, 5, "AuthorityOwnerInnerCreateNameRequestSendComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} sent InnerCreateNameRequest {3} to NameOwner {4}:{5}", "storeTraceId", "authorityOwnerNodeId", "authorityOwnerInstanceId", "name", "nameOwnerNodeId", "nameOwnerInstanceId"),
            AuthorityOwnerInnerCreateNameReplyReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 6, "AuthorityOwnerInnerCreateNameReplyReceiveComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} recieved InnerCreateNameReply {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateNameRequestReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 7, "NameOwnerInnerCreateNameRequestReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} received InnerCreateNameRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateNameRequestProcessComplete(Common::TraceTaskCodes::NamingStoreService, 8, "NameOwnerInnerCreateNameRequestProcessComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} processed InnerCreateNameRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerInnerDeleteNameRequestSendComplete(Common::TraceTaskCodes::NamingStoreService, 9, "AuthorityOwnerInnerDeleteNameRequestSendComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} sent InnerDeleteNameRequest {3} to NameOwner {4}:{5}", "storeTraceId", "authoritOwneryNodeId", "authorityOwnerInstanceId", "name", "nameOwnerNodeId", "nameOwnerInstanceId"),
            AuthorityOwnerInnerDeleteNameReplyReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 10, "AuthorityOwnerInnerDeleteNameReplyReceiveComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} recieved InnerDeleteNameReply {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteNameRequestReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 11, "NameOwnerInnerDeleteNameRequestReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} received InnerDeleteNameRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteNameRequestProcessComplete(Common::TraceTaskCodes::NamingStoreService, 12, "NameOwnerInnerDeleteNameRequestProcessComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} processed InnerDeleteNameRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerInnerCreateServiceRequestToNameOwnerSendComplete(Common::TraceTaskCodes::NamingStoreService, 13, "AuthorityOwnerInnerCreateServiceRequestToNameOwnerSendComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} sent InnerCreateServiceRequest {3} to NameOwner {4}:{5}", "storeTraceId", "authorityOwnerNodeId", "authorityOwnerInstanceId", "name", "nameOwnerNodeId", "nameOwnerInstanceId"),
            AuthorityOwnerInnerCreateServiceReplyFromNameOwnerReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 14, "AuthorityOwnerInnerCreateServiceReplyFromNameOwnerReceiveComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} recieved InnerCreateServiceReply {3} from Name Owner", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateServiceToFMSendComplete(Common::TraceTaskCodes::NamingStoreService, 15, "NameOwnerInnerCreateServiceToFMSendComplete", Common::LogLevel::Noise, "{0}: NameOwner at {1}:{2} sent InnerCreateServiceRequest {3} to FM", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateServiceReplyFromFMReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 16, "NameOwnerInnerCreateServiceReplyFromFMReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner at {1}:{2} recieved InnerCreateServiceReply {3} from FM", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateServiceRequestReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 17, "NameOwnerInnerCreateServiceRequestReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} received InnerCreateServiceRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerCreateServiceRequestProcessComplete(Common::TraceTaskCodes::NamingStoreService, 18, "NameOwnerInnerCreateServiceRequestProcessComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} processed InnerCreateServiceRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerInnerDeleteServiceRequestToNameOwnerSendComplete(Common::TraceTaskCodes::NamingStoreService, 19, "AuthorityOwnerInnerDeleteServiceRequestToNameOwnerSendComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} sent InnerDeleteServiceRequest {3} to NameOwner {4}:{5}", "storeTraceId", "authorityOwnerNodeId", "authorityOwnerInstanceId", "name", "nameOwnerNodeId", "nameOwnerInstanceId"),
            AuthorityOwnerInnerDeleteServiceReplyFromNameOwnerReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 20, "AuthorityOwnerInnerDeleteServiceReplyFromNameOwnerReceiveComplete", Common::LogLevel::Noise, "{0}: Authority Owner at {1}:{2} recieved InnerDeleteServiceReply {3} from Name Owner", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteServiceToFMSendComplete(Common::TraceTaskCodes::NamingStoreService, 21, "NameOwnerInnerDeleteServiceToFMSendComplete", Common::LogLevel::Noise, "{0}: NameOwner at {1}:{2} sent InnerDeleteServiceRequest {3} to FM", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteServiceReplyFromFMReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 22, "NameOwnerInnerDeleteServiceReplyFromFMReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner at {1}:{2} recieved InnerDeleteServiceReply {3} from FM", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteServiceRequestReceiveComplete(Common::TraceTaskCodes::NamingStoreService, 23, "NameOwnerInnerDeleteServiceRequestReceiveComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} received InnerDeleteServiceRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerInnerDeleteServiceRequestProcessComplete(Common::TraceTaskCodes::NamingStoreService, 24, "NameOwnerInnerDeleteServiceRequestProcessComplete", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} processed InnerDeleteServiceRequest for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            StartupRepairStarted(Common::TraceTaskCodes::NamingStoreService, 25, "StartupRepairStarted", Common::LogLevel::Noise, "{0}: Node {1}:{2} has started the startup repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerHierarchyRepairStarted(Common::TraceTaskCodes::NamingStoreService, 26, "AuthorityOwnerHierarchyRepairStarted", Common::LogLevel::Noise, "{0}: Authority {1}:{2} Owner has started the hierarchy repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerServiceRepairStarted(Common::TraceTaskCodes::NamingStoreService, 27, "AuthorityOwnerServiceRepairStarted", Common::LogLevel::Noise, "{0}: Authority Owner {1}:{2} has started the service repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerServiceRepairStarted(Common::TraceTaskCodes::NamingStoreService, 28, "NameOwnerServiceRepairStarted", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} has started the service repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            StartupRepairCompleted(Common::TraceTaskCodes::NamingStoreService, 29, "StartupRepairCompleted", Common::LogLevel::Noise, "{0}: Node {1}:{2} has completed the startup repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerHierarchyRepairCompleted(Common::TraceTaskCodes::NamingStoreService, 30, "AuthorityOwnerHierarchyRepairCompleted", Common::LogLevel::Noise, "{0}: Authority Owner {1}:{2} has completed the hierarchy repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            AuthorityOwnerServiceRepairCompleted(Common::TraceTaskCodes::NamingStoreService, 31, "AuthorityOwnerServiceRepairCompleted", Common::LogLevel::Noise, "{0}: Authority Owner {1}:{2} has completed the service repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name"),
            NameOwnerServiceRepairCompleted(Common::TraceTaskCodes::NamingStoreService, 32, "NameOwnerServiceRepairCompleted", Common::LogLevel::Noise, "{0}: NameOwner {1}:{2} has completed the service repair for name {3}", "storeTraceId", "nodeId", "instanceId", "name")
        {
        }  
    };
}
