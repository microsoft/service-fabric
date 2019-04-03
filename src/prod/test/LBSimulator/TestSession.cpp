// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestSession.h"
#include "TestDispatcher.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

const StringLiteral TestSession::LBSimulatorHelpFileName = "LBSimulatorHelp.txt";

TestSession::TestSession(wstring const& label, bool autoMode, TestDispatcher& testDispatcher)
    : TestCommon::TestSession(label, autoMode, testDispatcher, wstring(LBSimulatorHelpFileName.begin(), LBSimulatorHelpFileName.end())), 
    testDispatcher_(testDispatcher)
{
}

void TestSession::AddCommand(wstring const& command)
{
    AddInput(command);
}
