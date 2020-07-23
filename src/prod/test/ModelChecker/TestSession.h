// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "TestDispatcher.h"
#include "ModelCheckerConfig.h"

namespace ModelChecker
{
    class TestSession: public TestCommon::TestSession
    {
        DENY_COPY(TestSession)

    public:
        TestSession(std::wstring const& label, bool autoMode, TestDispatcher& testDispatcher);

        void AddCommand(std::wstring const& command);

        __declspec (property(get=get_Dispatcher)) TestDispatcher const& Dispatcher;
        TestDispatcher const & get_Dispatcher() const {return testDispatcher_;}

        __declspec (property(get=get_ModelCheckerConfig)) ModelCheckerConfig & ModelCheckerConfigObj;
        ModelCheckerConfig const& get_ModelCheckerConfig() const {return testDispatcher_.ModelCheckerConfigObj;}
        ModelCheckerConfig & get_ModelCheckerConfig() {return testDispatcher_.ModelCheckerConfigObj;}

    protected:
        TestDispatcher & testDispatcher_;
    };
};
