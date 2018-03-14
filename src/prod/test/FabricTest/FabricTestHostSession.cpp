// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace std;

const StringLiteral FabricTestHostSession::TraceSource = "FabricTestHostConfigSource";

FabricTestHostSessionSPtr * FabricTestHostSession::singleton_ = nullptr;

void FabricTestHostSession::AddCommand(std::wstring const & command)
{
    TestCommon::TestSession::AddInput(Command::CreateCommand(command));
}

FabricTestHostSession::FabricTestHostSession(
    wstring const& nodeId, 
    wstring const& label, 
    wstring const& clientId, 
    bool autoMode, 
    wstring const& serverListenAddress) 
    : fabricDispatcher_(),
    nodeId_(nodeId),
    TestCommon::ClientTestSession(
    serverListenAddress, 
    clientId, 
    label, 
    autoMode, 
    fabricDispatcher_, 
    L"")
{
}

void FabricTestHostSession::CreateSingleton(
    wstring const& nodeId, 
    wstring const& label, 
    wstring const& clientId, 
    bool autoMode, 
    wstring const& serverListenAddress)
{
    singleton_ = new shared_ptr<FabricTestHostSession>(new FabricTestHostSession(nodeId, label, clientId, autoMode, serverListenAddress));
}

FabricTestHostSession & FabricTestHostSession::GetFabricTestHostSession()
{
    return *(*FabricTestHostSession::singleton_);
}

DEFINE_SINGLETON_COMPONENT_CONFIG(FabricTestHostSessionConfig)
