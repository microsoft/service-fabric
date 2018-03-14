// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest::StateManagement;
using namespace ReliabilityUnitTest;

namespace
{
    template<typename T>
    shared_ptr<T> CreateConfiguration(std::wstring const & string)
    {
        auto store = make_shared<FileConfigStore>(string, true);
        return make_shared<T>(store);
    }
}

void ConfigurationManager::InitializeAssertConfig(bool isTestAssertEnabled)
{
    // Disable capturing stack traces 
    // Otherwise if compiled with fastlink dbghelp.dll hangs
    wstring testAssertConfig(wformatString("[Common]\r\nTestAssertEnabled={0}\r\nDebugBreakEnabled=false\r\nStackTraceCaptureEnabled=false\r\n", isTestAssertEnabled));
    Assert::LoadConfiguration(*CreateConfiguration<Common::Config>(testAssertConfig));
}

void ConfigurationManager::InitializeTraceConfig(
    int consoleTraceLevel,
    int fileTraceLevel,
    int etwTraceLevel)
{
    wstring config = wformatString("[Trace/Console]\r\nLevel={0}\r\n[Trace/ETW]\r\nLevel={1}\r\n[Trace/File]\r\nLevel={2}\r\nPath=RA.Test.trace", consoleTraceLevel, etwTraceLevel, fileTraceLevel);
    TraceProvider::Test_ReloadConfiguration(*CreateConfiguration<Common::Config>(config));
}
