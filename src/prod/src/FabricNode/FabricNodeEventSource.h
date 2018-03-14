// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Fabric
{
    class FabricNodeEventSource
    {
    public:
        
        Common::TraceEventWriter<std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring> NodeOpening;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> NodeOpened;
        Common::TraceEventWriter<std::wstring, uint64> NodeClosing;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> NodeClosed;
        Common::TraceEventWriter<std::wstring, uint64> NodeAborting;
        Common::TraceEventWriter<std::wstring, uint64> NodeAborted;
        Common::TraceEventWriter<std::wstring, uint64, Common::Guid> ResolvingNamingService;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> ResolvedNamingService;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> RetryingResolveNamingService;
        Common::TraceEventWriter<std::wstring, uint64, Common::TimeSpan> CreatingNamingService;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> CreatedNamingService;
        Common::TraceEventWriter<std::wstring, uint64, Common::ErrorCodeValue::Trace> RetryingCreateNamingService;
        Common::TraceEventWriter<std::wstring, uint64> StartingCreateNamingServiceTimer;
        Common::TraceEventWriter<std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, std::wstring> State;
        Common::TraceEventWriter<std::wstring, bool> ZombieModeState;
        Common::TraceEventWriter<std::wstring, Common::DateTime, int64, Common::DateTime, Common::DateTime> Time;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64> NodeOpeningOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64> NodeOpenedSuccessOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64, Common::ErrorCodeValue::Trace> NodeOpenedFailedOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64> NodeClosingOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, uint64, Common::ErrorCodeValue::Trace> NodeClosedOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64> NodeAbortingOperational;
		Common::TraceEventWriter<std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, bool, std::wstring, std::wstring, uint64> NodeAbortedOperational;

        FabricNodeEventSource() :
            NodeOpening(Common::TraceTaskCodes::FabricNode, 4, "_Nodes_NodeOpening", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} is opening with instance name: {2} upgrade domain: {3}, fault domain: {4}, address: {5}, hostname: {6}, isSeedNode: {7}, versionInstance: {8}", "id", "dca_instance", "instanceName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance"),
            NodeOpened(Common::TraceTaskCodes::FabricNode, 5, "_Nodes_NodeOpened", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} has opened with {2}.", "id", "dca_instance", "error"),
            NodeClosing(Common::TraceTaskCodes::FabricNode, 6, "_Nodes_NodeClosing", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} is closing.", "id", "dca_instance"),
            NodeClosed(Common::TraceTaskCodes::FabricNode, 7, "_Nodes_NodeClosed", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} has closed with {2}.", "id", "dca_instance", "error"),
            NodeAborting(Common::TraceTaskCodes::FabricNode, 8, "_Nodes_NodeAborting", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} is aborting.", "id", "dca_instance"),
            NodeAborted(Common::TraceTaskCodes::FabricNode, 9, "_Nodes_NodeAborted", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0}:{1} has aborted.", "id", "dca_instance"),
            ResolvingNamingService(Common::TraceTaskCodes::FabricNode, 10, "ResolvingNamingService", Common::LogLevel::Info, "{0}:{1} attempting to initialize naming service. Resolving {2} to see if service already exists.", "id", "instance", "namingServiceCuid"),
            ResolvedNamingService(Common::TraceTaskCodes::FabricNode, 11, "ResolvedNamingService", Common::LogLevel::Info, "{0}:{1} resolution of Naming Service during bootstrap attempt completed with {2}.", "id", "instance", "error"),
            RetryingResolveNamingService(Common::TraceTaskCodes::FabricNode, 12, "RetryingResolveNamingService", Common::LogLevel::Warning, "{0}:{1} retrying Naming Service resolution because of error {2}.", "id", "instance", "error"),
            CreatingNamingService(Common::TraceTaskCodes::FabricNode, 13, "CreatingNamingService", Common::LogLevel::Info, "{0}:{1} attempting to create naming service: retry = {2}.", "id", "instance", "retry"),
            CreatedNamingService(Common::TraceTaskCodes::FabricNode, 14, "CreatedNamingService", Common::LogLevel::Info, "{0}:{1} naming service creation completed  with {2}.", "id", "instance", "error"),
            RetryingCreateNamingService(Common::TraceTaskCodes::FabricNode, 15, "RetryingCreateNamingService", Common::LogLevel::Warning, "{0}:{1} retrying Naming Service creation because of error {2}.", "id", "instance", "error"),
            StartingCreateNamingServiceTimer(Common::TraceTaskCodes::FabricNode, 16, "StartingCreateNamingServiceTimer", Common::LogLevel::Info, "{0}:{1} Starting naming service creation timer.", "id", "instance"),
            State(Common::TraceTaskCodes::FabricNode, 17, "State", Common::LogLevel::Info, "{0}:{1} instance name: {9}, upgrade domain: {2}, fault domain: {3}, address: {4}, hostname: {5}, isSeedNode: {6}, state: {7}, versionInstance: {8}", "id", "dca_instance", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "state", "versionInstance", "instanceName"),
            ZombieModeState(Common::TraceTaskCodes::FabricNode, 18, "ZombieModeState", Common::LogLevel::Info, "{0} Node is in zombie mode: {1}", "nodeName", "isInZombieMode"),
            Time(Common::TraceTaskCodes::FabricNode, 19, "_GlobalTime_Time", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "{0} wall clock {1}, global time epoch {2}: {3}/{4}", "id", "localTime", "epoch", "lowerBound", "upperBound"),
			NodeOpeningOperational(Common::TraceTaskCodes::FabricNode, 20, "_NodesOperational_NodeOpeningOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} is opening with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance"),
			NodeOpenedSuccessOperational(Common::TraceTaskCodes::FabricNode, 21, "_NodesOperational_NodeOpenedSuccessOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} has successfully opened with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance"),
			NodeOpenedFailedOperational(Common::TraceTaskCodes::FabricNode, 22, "_NodesOperational_NodeOpenedFailedOperational", Common::LogLevel::Warning, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} has failed to open with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}, error: {9}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance", "error"),
			NodeClosingOperational(Common::TraceTaskCodes::FabricNode, 23, "_NodesOperational_NodeClosingOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} is closing with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance"),
			NodeClosedOperational(Common::TraceTaskCodes::FabricNode, 24, "_NodesOperational_NodeClosedOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} has closed with id: {1}, dca instance: {2}, error: {3}.", "nodeName", "id", "dca_instance", "error"),
			NodeAbortingOperational(Common::TraceTaskCodes::FabricNode, 25, "_NodesOperational_NodeAbortingOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} is aborting with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance"),
			NodeAbortedOperational(Common::TraceTaskCodes::FabricNode, 26, "_NodesOperational_NodeAbortedOperational", Common::LogLevel::Info, Common::TraceChannelType::Operational, Common::TraceKeywords::Enum::Default, "Node name: {0} has aborted with upgrade domain: {1}, fault domain: {2}, address: {3}, hostname: {4}, isSeedNode: {5}, versionInstance: {6}, id: {7}, dca instance: {8}", "nodeName", "upgradeDomain", "faultDomain", "address", "hostname", "isSeedNode", "versionInstance", "id", "dca_instance")
        {
        }
    };
}
