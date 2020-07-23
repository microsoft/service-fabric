// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "LBSimulatorConfig.h"
#include "PLB.h"
#include "Reliability\LoadBalancing\PlacementAndLoadBalancing.h"

namespace LBSimulator
{
    class FM;
    typedef std::shared_ptr<FM> FMSPtr;

    class TestSession;
    typedef std::shared_ptr<TestCommon::TestSession> SessionSPtr;

    class TraceParser;
    class Reliability::LoadBalancingComponent::FailoverUnitMovement;
    
    class TestDispatcher : public TestCommon::TestDispatcher
    {
        DENY_COPY(TestDispatcher)

    public:
        TestDispatcher();

        static std::wstring const VerifyCommand;
        static std::wstring const CreateNodeCommand;
        static std::wstring const DeleteNodeCommand;
        static std::wstring const CreateServiceCommand;
        static std::wstring const DeleteServiceCommand;
        static std::wstring const ReportLoadCommand;
        static std::wstring const ReportBatchLoadCommand;
        static std::wstring const ReportBatchMoveCostCommand;
        static std::wstring const LoadPlacementCommand;
        static std::wstring const LoadRandomPlacementCommand;
        static std::wstring const LoadPlacementDumpCommand;
        static std::wstring const ExecutePLBCommand;
        static std::wstring const ExecutePlacementDumpActionCommand;
        static std::wstring const ResetCommand;
        static std::wstring const SetDefaultMoveCostCommand;
        static std::wstring const ShowAllCommand;
        static std::wstring const ShowServicesCommand;
        static std::wstring const ShowNodesCommand;
        static std::wstring const ShowGFUMCommand;
        static std::wstring const ShowFailoverUnitsCommand;
        static std::wstring const ShowPlacementCommand;
        static std::wstring const SetPLBConfigCommand;
        static std::wstring const GetPLBConfigCommand;
        static std::wstring const DeleteReplicaCommand;
        static std::wstring const PromoteReplicaCommand;
        static std::wstring const MoveReplicaCommand;
        static std::wstring const ForceRefreshCommand;
        static std::wstring const PrintClusterStateCommand;
        static std::wstring const LogMetricsCommand;
        static std::wstring const PLBBatchRunsCommand;
        static std::wstring const FlushTracesCommand;
        static std::wstring const ConvertNodeIdCommand;
        static std::wstring const LoadPlacementFromTracesCommand;
        static std::wstring const PlaceholderFunctionCommand;

        __declspec (property(get = get_LBSimulatorConfig)) LBSimulatorConfig & LBSimulatorConfigObj;
        LBSimulatorConfig const& get_LBSimulatorConfig() const { return LBSimulatorConfig::GetConfig(); }
        LBSimulatorConfig & get_LBSimulatorConfig() { return LBSimulatorConfig::GetConfig(); }
        typedef Reliability::LoadBalancingComponent::PLBConfig PLBConfig;
        __declspec (property(get = get_PLBConfig)) PLBConfig & PLBConfigObj;
        PLBConfig const & get_PLBConfig() const { return PLBConfig::GetConfig(); }
        PLBConfig & get_PLBConfig() { return PLBConfig::GetConfig(); }

        virtual bool Open();
        virtual void Close();
        virtual bool ExecuteCommand(std::wstring command);
        virtual std::wstring GetState(std::wstring const & param);

    private:
        bool Verify(Common::StringCollection const & params);
        bool CreateNode(Common::StringCollection const & params);
        bool DeleteNode(Common::StringCollection const & params);
        bool CreateServices(Common::StringCollection const & params);
        bool DeleteService(Common::StringCollection const & params);
        bool ReportLoad(Common::StringCollection const & params);
        bool ReportBatchLoad(Common::StringCollection const & params);
        bool ReportBatchMoveCost(Common::StringCollection const & params);
        bool LoadPlacement(Common::StringCollection const & params);
        bool LoadRandomPlacement(Common::StringCollection const & params);
        bool LoadPlacementDump(Common::StringCollection const & params);
        bool ExecutePLB(Common::StringCollection const & params);
        bool ExecutePlacementDumpAction(Common::StringCollection const & params);
        bool Reset(Common::StringCollection const & params);
        bool SetDefaultMoveCost(Common::StringCollection const & params);
        bool ShowAll(Common::StringCollection const & params);
        bool ShowServices(Common::StringCollection const & params);
        bool ShowNodes(Common::StringCollection const & params);
        bool ShowGFUM(Common::StringCollection const & params);
        bool ShowPlacement(Common::StringCollection const & params);
        bool SetPLBConfig(Common::StringCollection const & params);
        bool GetPLBConfig(Common::StringCollection const & params);
        bool DeleteReplica(Common::StringCollection const & params);
        bool PromoteReplica(Common::StringCollection const & params);
        bool MoveReplica(Common::StringCollection const & params);
        bool ForceRefresh(Common::StringCollection const & params);
        bool PrintClusterState(Common::StringCollection const & params);
        bool LogMetrics(Common::StringCollection const & params);
        bool PLBBatchRuns(Common::StringCollection const & params);
        bool FlushTraces(Common::StringCollection const & params);
        bool ConvertNodeId(Common::StringCollection const & params);
        bool LoadPlacementFromTraces(Common::StringCollection const & params);
        bool PlaceholderFunction(Common::StringCollection const & params);
        bool TogglePLB(bool turnOn = true);
        bool ForcePLBSync();

        FMSPtr fm_;
        PLB plb_;
        int traceFileVersion_;
    };
}
