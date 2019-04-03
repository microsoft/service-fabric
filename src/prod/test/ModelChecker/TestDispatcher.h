// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "ModelCheckerConfig.h"
#include "Reliability/Failover/Failover.h"
#include "Reliability/Failover/FMStateMachineTask.h"
#include "Reliability/Failover/FMStateUpdateTask.h"
#include "Reliability/Failover/FMStatelessCheckTask.h"
#include "Reliability/Failover/FMPendingTask.h"
#include "Reliability/Failover/Epoch.h"
#include "Reliability/Failover/FMFailoverUnit.h"
#include "Reliability/Failover/ReplicaInfo.h"

#include <stack>
#include <unordered_set>
#include "State.h"
#include "Root.h"
#include "FauxCache.h"
#include "StateSpaceExplorer.h"

namespace ModelChecker{

    class TestSession;
    typedef std::tr1::shared_ptr<TestCommon::TestSession> SessionSPtr;
    

    class TestDispatcher: public TestCommon::TestDispatcher
    {
        DENY_COPY(TestDispatcher)
            
    public:
        static std::wstring const LoadModelCommand;
        static std::wstring const VerifyCommand;
        static std::wstring const TestStackCommand;
        static std::wstring const FMStateMachineTestCommand;

        // Starts dfs of state-space
        static std::wstring const CheckModelCommand; 

        static std::wstring const ParamDelimiter;
        static std::wstring const ItemDelimiter;
        static std::wstring const PairDelimiter;

        static std::wstring const EmptyCharacter;

        static Common::StringLiteral const TraceSource;

        static bool Bool_Parse(std::wstring const & input);

        static Common::StringCollection Split(std::wstring const& input, std::wstring const& delimiter);
        static Common::StringCollection Split(std::wstring const& input, std::wstring const& delimiter, std::wstring const& emptyChar);

        TestDispatcher();

        virtual bool Open();

        virtual void Close();

        virtual bool ExecuteCommand(std::wstring command);

        virtual std::wstring GetState(std::wstring const & param);

        __declspec (property(get=get_ModelCheckerConfig)) ModelCheckerConfig & ModelCheckerConfigObj;
        ModelCheckerConfig const& get_ModelCheckerConfig() const {return ModelCheckerConfig::GetConfig();}
        ModelCheckerConfig & get_ModelCheckerConfig() {return ModelCheckerConfig::GetConfig();}

    private:
        bool LoadModel(Common::StringCollection const & params);

        bool Verify(Common::StringCollection const & params);

        Federation::NodeId CreateNodeId(int id);

        Federation::NodeInstance CreateNodeInstance(int id, uint64 instance);

        Reliability::Epoch EpochFromString(std::wstring const& value);

        Reliability::FailoverManagerComponent::FailoverUnit::Flags FailoverUnitFlagsFromString(std::wstring const& value);

        Reliability::FailoverManagerComponent::FailoverUnitUPtr EmptyFailoverUnitFromString(std::wstring const& failoverUnitStr);

        bool CheckTheModel(Common::StringCollection const & params);

        void InitializeRoot();

        State InitialState();

        State CreateInitialState();

        // Contains Transport, HealthClient, FederationSubsystem, and FM
        RootSPtr root_;

        // Like statelessTasks in FMStateMachineTask.Test
        std::vector<Reliability::FailoverManagerComponent::StateMachineTaskUPtr> statelessTasks_;

        // Initial state
        State s0_;

        StateSpaceExplorer stateSpaceExplorer_;
    };
}
