// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <thread>
#include "TestCommon.h"
#include "PerfTest.h"

using namespace Transport;
using namespace Common;
using namespace std;

RealConsole console;

#ifdef PLATFORM_UNIX
int main(int argc, char* argv[])
#else
int wmain(int argc, wchar_t* argv[])
#endif
{
    Config config; //trigger config loading
    //TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::Console, LogLevel::Warning); 
    //TraceProvider::GetSingleton()->AddFilter(TraceSinkType::Console, TransportPerfTestConsoleTraceFilter);
    TTestUtil::DisableSendThrottling();

    if (argc != 3)
    {
        console.WriteLine("usage: {0} <address> <SecurityProvider>", argv[0]);
        return -1;
    }

#ifdef PLATFORM_UNIX
    auto destAddress = StringUtility::Utf8ToUtf16(string(argv[1]));
    auto providerString = StringUtility::Utf8ToUtf16(string(argv[2]));
#else
    auto destAddress = wstring(argv[1]);
    auto providerString = wstring(argv[2]);
#endif

    console.WriteLine("connecting to {0} with security provider {1}", destAddress, providerString);

    SecurityProvider::Enum provider = SecurityProvider::None;
    auto error = SecurityProvider::FromCredentialType(providerString, provider);
    ASSERT_IFNOT(error.IsSuccess(), "SecurityProvider::FromCredentialType({0}) failed: {1}", providerString, error);

    ProtectionLevel::Enum pl;
    error = ProtectionLevel::Parse(SecurityConfig::GetConfig().ClientServerProtectionLevel, pl);
    ASSERT_IFNOT(error.IsSuccess(), "ProtectionLevel::Parse({0}) failed: {1}", SecurityConfig::GetConfig().ClientServerProtectionLevel, error);

    SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(provider, pl);
    console.WriteLine("security settings: {0}", securitySettings);

    auto sender = TcpDatagramTransport::CreateClient();
    Invariant(sender->SetSecurity(securitySettings).IsSuccess());

    ManualResetEvent shouldStart;
    ManualResetEvent shouldStop;
    PerfTestParameters testParameters;
    sender->SetMessageHandler(
        [&](MessageUPtr & msg, ISendTarget::SPtr const &)
        {
            if (!shouldStart.IsSet())
            {
                Invariant(msg->GetBody(testParameters));
                shouldStart.Set();
                return;
            }

            shouldStop.Set();
        });

    sender->SetConnectionFaultHandler([](ISendTarget const &, ErrorCode fault)
    {
        console.WriteLine("connection closed: {0}", fault);
        ExitProcess(1);
    });

    Invariant(sender->Start().IsSuccess());

    //send a message to retrieve test parameters
    ISendTarget::SPtr target = sender->ResolveTarget(destAddress);
    auto msg = make_unique<Message>();
    msg->Headers.Add(MessageIdHeader());
    sender->SendOneWay(target, move(msg));

    if (!shouldStart.WaitOne(TimeSpan::FromSeconds(10)))
    {
        console.WriteLine("failed to retrieve test parameters");
        return -2;
    }

    console.WriteLine("test parameters: {0}", testParameters);

    vector<char> buffer(testParameters.MessageSize());
    vector<const_buffer> buffers;
    buffers.push_back(const_buffer(buffer.data(), buffer.size()));

    TTestUtil::ReduceTracingInFreBuild();
    TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::Console, LogLevel::Enum::Warning);

    const uint sendThrottle = 1024 * 1024 * 1024; // 1 GB
    wstring testAction = TTestUtil::GetGuidAction();
    if (testParameters.ThreadCount() == 1)
    {
        for (uint i = 0; i < testParameters.MessageCount(); ++i)
        {
            auto msg1 = make_unique<Message>(buffers, [](vector<const_buffer> const &, void*) {}, nullptr);
            msg1->Headers.Add(ActorHeader(Actor::GenericTestActor));
            msg1->Headers.Add(ActionHeader(testAction));
            msg1->Headers.Add(MessageIdHeader());
            error = sender->SendOneWay(target, move(msg1));
            Invariant(error.IsSuccess());

            if (target->BytesPendingForSend() > sendThrottle)
            {
                console.WriteLine("send throttled");
                this_thread::yield();
            }
        }
    }
    else
    {
        atomic_uint64 sendCount(0);
        for (uint i = 0; i < testParameters.ThreadCount(); ++i)
        {
            Threadpool::Post([&]
            {
                while ((++sendCount) <= testParameters.MessageCount())
                {
                    auto msg1 = make_unique<Message>(buffers, [](vector<const_buffer> const &, void*) {}, nullptr);
                    msg1->Headers.Add(ActorHeader(Actor::GenericTestActor));
                    msg1->Headers.Add(ActionHeader(testAction));
                    msg1->Headers.Add(MessageIdHeader());
                    auto error = sender->SendOneWay(target, move(msg1));
                    Invariant(error.IsSuccess());

                    if (target->BytesPendingForSend() > sendThrottle)
                    {
                        console.WriteLine("send throttled");
                        this_thread::yield();
                    }
                }
            });
        }
    }

    shouldStop.WaitOne();
    console.WriteLine("test completed");
    return 0;
}
