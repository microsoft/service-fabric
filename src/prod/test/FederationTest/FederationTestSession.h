// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
#define FEDERATIONSESSION  \
    FederationTestSession::GetFederationTestSession()    \

    class FederationTestSession;
    typedef std::shared_ptr<FederationTestSession> FederationTestSessionSPtr;

    class FederationTestSession: public TestCommon::TestSession
    {
        DENY_COPY(FederationTestSession)

    public:

        static void CreateSingleton(std::wstring label, bool autoMode);
        static FederationTestSession & GetFederationTestSession();

        void AddCommand(std::wstring command);

        __declspec (property(get=getFederationDispatcher)) FederationTestDispatcher & FederationDispatcher;

        FederationTestDispatcher & getFederationDispatcher()
        {
            return rpDispatcher_;
        }

    protected:
        FederationTestSession(std::wstring label, bool autoMode) 
            : rpDispatcher_(), TestSession(label, autoMode, rpDispatcher_)
        {}

        static FederationTestSessionSPtr * singleton_;

    private:
        static const Common::StringLiteral traceSource_;
        FederationTestDispatcher rpDispatcher_;
    };
};
