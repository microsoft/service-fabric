// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "TestDispatcher.h"
#include "LBSimulatorConfig.h"

namespace LBSimulator
{
    class TestSession: public TestCommon::TestSession
    {
        DENY_COPY(TestSession)

    public:
        TestSession(std::wstring const& label, bool autoMode, TestDispatcher& testDispatcher);

        void AddCommand(std::wstring const& command);

        __declspec (property(get=get_Dispatcher)) TestDispatcher const& Dispatcher;
        TestDispatcher const & get_Dispatcher() const {return testDispatcher_;}

        __declspec (property(get=get_LBSimulatorConfig)) LBSimulatorConfig & LBSimulatorConfigObj;
        LBSimulatorConfig const& get_LBSimulatorConfig() const {return testDispatcher_.LBSimulatorConfigObj;}
        LBSimulatorConfig & get_LBSimulatorConfig() {return testDispatcher_.LBSimulatorConfigObj;}

        typedef Reliability::LoadBalancingComponent::PLBConfig PLBConfig;
        __declspec (property(get=get_PLBConfig)) PLBConfig & PLBConfigObj;
        PLBConfig const & get_PLBConfig() const {return testDispatcher_.PLBConfigObj;}
        PLBConfig & get_PLBConfig() {return testDispatcher_.PLBConfigObj;}

    protected:
        TestDispatcher & testDispatcher_;

        static const Common::StringLiteral LBSimulatorHelpFileName;
    };
};
