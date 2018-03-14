// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace FederationTest
{
    const Common::StringLiteral FederationTestSession::traceSource_ = "FederationTestConfigSource";

    FederationTestSessionSPtr * FederationTestSession::singleton_ = nullptr;

    void FederationTestSession::AddCommand(std::wstring command)
    {
        TestCommon::TestSession::AddInput(command);
    }

    void FederationTestSession::CreateSingleton(wstring label, bool autoMode)
    {
        singleton_ = new shared_ptr<FederationTestSession>(new FederationTestSession(label, autoMode));
    }

    FederationTestSession & FederationTestSession::GetFederationTestSession()
    {
        return *(*FederationTestSession::singleton_);
    }
}
