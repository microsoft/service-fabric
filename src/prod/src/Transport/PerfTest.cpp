// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestCommon.h"
#include "PerfTest.h"

using namespace Transport;
using namespace Common;
using namespace std;

RealConsole console;

class PerfTest
{
public:
    PerfTest(
        SecurityProvider::Enum secProvider,
        uint messageSize,
        uint clientThreadCount);

    ~PerfTest();

    void Run(FileWriter & sw);
    TimeSpan GetTestDuration() const;

    static void ParseCmdline(int argc, wchar_t* argv[]);
    static void SetupTest(bool installCerts);
    static void CleanupTest(bool uninstallCerts);

    static void RunRecvBufferSizeTests(SecurityProvider::Enum secProvider);
    static void SetShouldQueueReceivedMessage(bool value);

private:
    void StartListener();
    void StartClient();
    void StartClientOnRemoteComputer();
    void WaitForResult(FileWriter & sw);
    static void PrintUsageAndExit();

    IDatagramTransportSPtr listener_;
    SecurityProvider::Enum securityProvider_;
    atomic_uint64 recvCount_{0};
    atomic_uint64 recvBytes_{0};
    PerfTestParameters testParameters_;
    Stopwatch stopwatch_;
    AutoResetEvent allReceived_;
    TimeSpan testTimeout_;

    RwLock receiveQueueLock;
    queue<MessageUPtr> receiveQueue;

    static uint tcpBufferSizeMin_;
    static uint tcpBufferSizeMax_;
    static uint testDataSize_;
    static uint clientThreadMin_;
    static uint clientThreadMax_;
    static uint messageSizeMin_;
    static uint messageSizeMax_;
    static bool shouldQueueReceivedMessage_;

    static uint runCount_;
};

static const uint bsizeMinDefault = 64 * 1024; // 64 KB
uint PerfTest::tcpBufferSizeMin_ = bsizeMinDefault;
static const uint bsizeMaxDefault = 128 * 1024; // 128 KB
uint PerfTest::tcpBufferSizeMax_ = bsizeMaxDefault;

#ifdef DBG
static const uint dataSizeDefault = 16 * 1024 * 1024; // 16 MB
#else
static const uint dataSizeDefault = 128 * 1024 * 1024; // 128 MB
#endif

uint PerfTest::testDataSize_ = dataSizeDefault;

static const uint tminDefault = 1;
uint PerfTest::clientThreadMin_ = tminDefault;
static const uint tmaxDefault = 3;
uint PerfTest::clientThreadMax_ = tmaxDefault;
static const uint msizeMinDefault = 128;
uint PerfTest::messageSizeMin_ = msizeMinDefault;
static const uint msizeMaxDefault = 512 * 1024; // 512 KB
uint PerfTest::messageSizeMax_ = msizeMaxDefault;

#ifdef DBG
static const uint messageCountMin = 2000;
#else
static const uint messageCountMin = 30000;
#endif

static const bool qrDefault = true;
bool PerfTest::shouldQueueReceivedMessage_ = qrDefault;

uint PerfTest::runCount_ = 0;

static const wstring clientExeName(L"Transport.PerfTest.Client.exe");
static const wstring certSetupExe(L"RingCertSetup.exe");
static const wstring lib0(L"FabricResources.dll");
static const wstring lib1(L"FabricCommon.dll");
static const wstring lib2(L"boost_unit_test_framework.dll");
static const wstring lib3(L"boost_unit_test_framework-vc140-mt-1_61.dll");

static wstring computer;
static wstring remotePath;
static wstring remoteWorkDir;

bool securityProviderSet = false;
SecurityProvider::Enum securityProvider = SecurityProvider::None;
ProtectionLevel::Enum protectionLevel = ProtectionLevel::EncryptAndSign;

#ifdef PLATFORM_UNIX
int main(int argc, char* argva[])
#else
void wmain(int argc, wchar_t* argv[])
#endif
{
    Config config; // Trigger config loading
    TraceProvider::LoadConfiguration(config);
    //TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::Console, LogLevel::Warning); 
    //TraceProvider::GetSingleton()->AddFilter(TraceSinkType::Console, TransportPerfTestConsoleTraceFilter);

#ifdef PLATFORM_UNIX
    Invariant(argc >= 1);
    vector<wstring> args;
    for(int i = 0; i < argc; ++i)
    {
        args.emplace_back(StringUtility::Utf8ToUtf16(argva[i]));
    }

    vector<wchar_t*> argvv;
    for(auto const & arg : args)
    {
        argvv.emplace_back(const_cast<wchar_t*>(arg.c_str()));
    }

    auto argv = argvv.data();
#endif

    PerfTest::ParseCmdline(argc, argv);

    bool certs = !securityProviderSet || (securityProvider == SecurityProvider::Ssl);
    if (certs)
    {
        console.WriteLine("\n!!! please run RingCertSetup.exe to install certificates !!!");
        if (!remotePath.empty())
        {
            console.WriteLine("!!! please run RingCertSetup.exe to install certificates on remote machine !!!");
        }

        // skip auto cert installing/uninstalling until we figure out all dependencies of RingCertSetup.exe 
        certs = false; 
    }

    PerfTest::SetupTest(certs);

    if (securityProviderSet)
    {
        PerfTest::RunRecvBufferSizeTests(securityProvider);
    }
    else
    {
        PerfTest::RunRecvBufferSizeTests(SecurityProvider::None);
        PerfTest::RunRecvBufferSizeTests(SecurityProvider::Ssl);
    }

    PerfTest::CleanupTest(certs);
}

PerfTest::PerfTest(
    SecurityProvider::Enum secProvider,
    uint messageSize,
    uint clientThreadCount)
    : securityProvider_(secProvider)
    , testParameters_(
        messageSize,
        (testDataSize_ / messageSize) >= messageCountMin ? (testDataSize_ / messageSize) : messageCountMin,
        clientThreadCount)
    , testTimeout_(TimeSpan::FromMinutes(10))
{
    TTestUtil::ReduceTracingInFreBuild();
}

PerfTest::~PerfTest()
{
    TTestUtil::RecoverTracingInFreBuild();
}

struct TestRoot : public ComponentRoot
{
};

void PerfTest::StartListener()
{
    wstring localFqdn;
    auto error = TcpTransportUtility::GetLocalFqdn(localFqdn);
    Invariant(error.IsSuccess());

    for (USHORT port = 22000; port < 23000; ++port)
    {
        listener_ = TcpDatagramTransport::Create(wformatString("{0}:{1}", localFqdn, port));
        SecuritySettings securitySettings = TTestUtil::CreateTestSecuritySettings(securityProvider_, protectionLevel);
        Invariant(listener_->SetSecurity(securitySettings).IsSuccess());

        auto root = make_shared<TestRoot>(); // make it real!
        listener_->SetMessageHandler([this, root](MessageUPtr & receivedMsg, ISendTarget::SPtr const & st)
        {
            auto after = ++recvCount_;
            recvBytes_ += receivedMsg->SerializedSize();
            if (after == 1)
            {
                console.WriteLine("send test parameters to {0}:{1}", st->Address(), testParameters_);
                auto msg = make_unique<Message>(testParameters_);
                listener_->SendOneWay(st, move(msg));
                return;
            }

            if (after == 2)
            {
                stopwatch_.Start();
                if (after < (testParameters_.MessageCount() + 1))
                {
                    return;
                }
            }

            if (after == (testParameters_.MessageCount() + 1))
            {
                stopwatch_.Stop();
                allReceived_.Set();

                // stop sending side
                auto msg = make_unique<Message>();
                listener_->SendOneWay(st, move(msg));
                return;
            }

            if (shouldQueueReceivedMessage_)
            {
                AcquireWriteLock grab(receiveQueueLock);

                receiveQueue.push(receivedMsg->Clone());

                if (receiveQueue.size() > 8)
                {
                    receiveQueue.pop();
                }
            }
        });

        error = listener_->Start();
        if (!error.IsSuccess())
        {
            Invariant(error.IsError(ErrorCodeValue::AddressAlreadyInUse));
            console.WriteLine("!!! address conflict, will retry !!!");
            continue;
        }

        console.WriteLine("listen address: {0}", listener_->ListenAddress());
        break;
    }
}

void PerfTest::StartClient()
{
#ifdef PLATFORM_UNIX
    console.WriteLine("client process needs to be started manually on Linux");
#else

    if (!computer.empty())
    {
        StartClientOnRemoteComputer();
        return;
    }

    auto cmdline = wformatString(
        "{0} {1} {2}",
        Path::Combine(Environment::GetExecutablePath(), clientExeName),
        listener_->ListenAddress(),
        securityProvider_);

    console.WriteLine("{0}", cmdline);

    HandleUPtr processHandle, threadHandle;
    vector<wchar_t> envBlock = vector<wchar_t>();

    auto error = ProcessUtility::CreateProcess(
        cmdline,
        wstring(),
        envBlock,
        0,
        processHandle,
        threadHandle);

    Invariant(error.IsSuccess());
#endif
}

void PerfTest::StartClientOnRemoteComputer()
{
#ifndef PLATFORM_UNIX
    wstring cmdline = wformatString(
        "{0} {1} {2}",
        clientExeName,
        listener_->ListenAddress(),
        securityProvider_);

    console.WriteLine("{0}", cmdline);
    TTestUtil::CreateRemoteProcess(
        computer,
        Path::Combine(remoteWorkDir, cmdline),
        remoteWorkDir,
        0);
#endif
}

void PerfTest::WaitForResult(FileWriter & fw)
{
    Invariant(allReceived_.WaitOne(testTimeout_));
    auto elapsedMilliseconds = stopwatch_.ElapsedMilliseconds;
    console.WriteLine(">>> time elapsed: {0} ms", elapsedMilliseconds);
    auto totalReceivedBytes = recvBytes_.load();
    auto recvRate = (totalReceivedBytes * 8.0) / elapsedMilliseconds / 1000.0;
    console.WriteLine(">>> received: {0} bytes", totalReceivedBytes);
    console.WriteLine(">>> receive rate: {0} mbps\n\n", recvRate);

    fw.Write(",{0},{1}", elapsedMilliseconds, recvRate);

    listener_->Stop();
    listener_.reset();
}

void PerfTest::Run(FileWriter & fw)
{
    ++runCount_;
    console.WriteLine("<<< starting test round {0} at {1:local}", runCount_, DateTime::Now());
    Trace.WriteInfo(TraceType, "<<< starting test round {0} at {1:local}", runCount_, DateTime::Now());

    StartListener();
    StartClient();
    WaitForResult(fw);
}

void PerfTest::RunRecvBufferSizeTests(SecurityProvider::Enum secProvider)
{
    console.WriteLine("=========================================================");
    console.WriteLine("security provider = {0}", secProvider);
    console.WriteLine(" test data total in bytes: {0}", testDataSize_);
    console.WriteLine("=========================================================");

    for (uint clientThreadCount = clientThreadMin_; clientThreadCount <= clientThreadMax_; ++clientThreadCount)
    {
        wstring outputFile = wformatString(
            "PerfTest-QueueReceived@{0}_ClientThread@{1}_Sec@{2}.csv",
            shouldQueueReceivedMessage_, clientThreadCount, secProvider);

        console.WriteLine("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
        console.WriteLine("client thread count = {0}", clientThreadCount);
        console.WriteLine("ShouldQueueReceivedMessage = {0}", shouldQueueReceivedMessage_);
        console.WriteLine("output = {0}", outputFile);
        console.WriteLine("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

        FileWriter csvFile;
        auto error = csvFile.TryOpen(outputFile);
        Invariant(error.IsSuccess());
        KFinally([&] { csvFile.Close(); });

        for (uint messageSize = messageSizeMin_; messageSize <= messageSizeMax_; messageSize *= 2)
        {
            csvFile.Write("{0}", messageSize);

            map<TimeSpan, uint> resultMap;
            for (uint tcpBufferSize = tcpBufferSizeMin_; tcpBufferSize <= tcpBufferSizeMax_; tcpBufferSize += 4*1024)
            {
                TransportConfig::GetConfig().TcpReceiveBufferSize = tcpBufferSize;
                console.WriteLine("---------------------------------------------------------");
                console.WriteLine("tcp receive buffer = {0}", TransportConfig::GetConfig().TcpReceiveBufferSize);
                console.WriteLine("---------------------------------------------------------");

                //csvFile.Write("{0}", tcpBufferSize);
                PerfTest perfTest(secProvider, messageSize, clientThreadCount);
                perfTest.Run(csvFile);
                resultMap.emplace(make_pair(perfTest.GetTestDuration(), tcpBufferSize));
            }

            // record tcpBufferSize in KB, sorted by test duration
            console.WriteLine("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
            console.WriteLine("TCP buffer sizes sorted by test duration:");
            auto optimalBufferSize = resultMap.cbegin()->second / 1024;
            for (auto const & entry : resultMap)
            {
                auto bufferSize = entry.second/1024;
                csvFile.Write(",{0}", bufferSize); 
                console.WriteLine("{0}", bufferSize); 
            }
            csvFile.WriteLine();
            csvFile.Flush();

            console.WriteLine("\n@_@ optimal buffer size = {0} @_@", optimalBufferSize);
            console.WriteLine("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
        }
    }
}

TimeSpan PerfTest::GetTestDuration() const
{
    return stopwatch_.Elapsed;
}

void PerfTest::SetShouldQueueReceivedMessage(bool value)
{
    shouldQueueReceivedMessage_ = value;
}

static const wstring remoteArg = L"-remote";
static const wstring bsizeMinArg = L"-bsizeMin";
static const wstring bsizeMaxArg = L"-bsizeMax";
static const wstring dataArg = L"-data";
static const wstring tminArg = L"-tmin";
static const wstring tmaxArg = L"-tmax";
static const wstring mminArg = L"-mmin";
static const wstring mmaxArg = L"-mmax";
static const wstring qrArg = L"-qr";
static const wstring securityArg = L"-security";

void PerfTest::ParseCmdline(int argc, wchar_t* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        wstring arg(argv[i]);
        vector<wstring> tokens;
        StringUtility::Split<wstring>(arg, tokens, L":");
        if ((tokens.size() != 2) || !StringUtility::StartsWith(tokens.front(), L"-"))
        {
            console.WriteLine("arg[{0}] = '{1}', token count = {2}, 2 expected", i, arg, tokens.size());
            if (!tokens.empty())
            {
                console.WriteLine("tokens: {0}", tokens);
            }
            PrintUsageAndExit();
        }

#ifndef PLATFORM_UNIX
        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), remoteArg))
        {
            remotePath = tokens[1];
            console.WriteLine("remote path = {0}", remotePath);
            if (!StringUtility::StartsWith(remotePath, L"\\\\"))
            {
                console.WriteLine("Invalid UNC path: {0}", remotePath);
                PrintUsageAndExit();
            }

            auto pos = remotePath.find_first_of(L"\\", 2);
            computer = remotePath.substr(0, pos);
            if (computer.empty())
            {
                console.WriteLine("Invalid UNC path: {0}", remotePath);
                PrintUsageAndExit();
            }

            if (pos != wstring::npos)
            {
                remoteWorkDir = remotePath.substr(pos + 1, remotePath.length() - pos - 1);
            }

            if (remoteWorkDir.empty() || (remoteWorkDir[1] != L'$'))
            {
                console.WriteLine("Invalid UNC path: {0}", remotePath);
                PrintUsageAndExit();
            }

            remoteWorkDir[1] = L':';
            console.WriteLine("remote computer = {0}", computer);
            console.WriteLine("remote work directory = {0}", remoteWorkDir);

            continue;
        }
#endif

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), bsizeMinArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], tcpBufferSizeMin_))
            {
                console.WriteLine("Failed to parse '{0}' as buffer size", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), bsizeMaxArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], tcpBufferSizeMax_))
            {
                console.WriteLine("Failed to parse '{0}' as buffer size", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), dataArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], testDataSize_))
            {
                console.WriteLine("Failed to parse '{0}' as data size", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), tminArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], clientThreadMin_))
            {
                console.WriteLine("Failed to parse '{0}' as thread count", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), tmaxArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], clientThreadMax_))
            {
                console.WriteLine("Failed to parse '{0}' as thread count", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), mminArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], messageSizeMin_))
            {
                console.WriteLine("Failed to parse '{0}' as message size", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), mmaxArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], messageSizeMax_))
            {
                console.WriteLine("Failed to parse '{0}' as message size", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), qrArg))
        {
            if (!StringUtility::TryFromWString(tokens[1], shouldQueueReceivedMessage_))
            {
                console.WriteLine("Failed to parse '{0}' as boolean", tokens[1]); 
                PrintUsageAndExit();
            }
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(tokens.front(), securityArg))
        {
            if (!SecurityProvider::FromCredentialType(tokens[1], securityProvider).IsSuccess())
            {
                console.WriteLine("Failed to parse '{0}' as SecurityProvider", tokens[1]); 
                PrintUsageAndExit();
            }

            auto plStr = SecurityConfig::GetConfig().ClientServerProtectionLevel;
            auto error = ProtectionLevel::Parse(plStr, protectionLevel);
            if (!error.IsSuccess())
            {
                console.WriteLine("Failed to parse '{0}' as SecurityProvider", tokens[1]); 
                PrintUsageAndExit();
            }

            securityProviderSet = true;
            continue;
        }

        PrintUsageAndExit();
    }

    // validate
    if (tcpBufferSizeMin_ > tcpBufferSizeMax_)
    {
        console.WriteLine("Invalid TCP buffer range: [{0}, {1}]", tcpBufferSizeMin_, tcpBufferSizeMax_);
        ::ExitProcess(1);
    }

    if (clientThreadMin_ > clientThreadMax_)
    {
        console.WriteLine("Invalid client thread range: [{0}, {1}]", clientThreadMin_, clientThreadMax_);
        ::ExitProcess(1);
    }

    if (messageSizeMin_ > messageSizeMax_)
    {
        console.WriteLine("Invalid message size range: [{0}, {1}]", messageSizeMin_, messageSizeMax_);
        ::ExitProcess(1);
    }
}

void PerfTest::PrintUsageAndExit()
{
    console.WriteLine("Usage: {0} [-option:value] ...", Path::GetFileName(Environment::GetExecutableFileName()));
    console.WriteLine("{0}:remote UNC path to copy client exe, default to empty for local run.", remoteArg);
    console.WriteLine("        if specified, must be a full path so that remote work dir can be derived.");
    console.WriteLine("        for exampe: \\\\MachineName\\c$\\temp");
    console.WriteLine("{0}:minimal TCP buffer size, default to {1}", bsizeMinArg, bsizeMinDefault);
    console.WriteLine("{0}:maximal TCP buffer size, default to {1}", bsizeMaxArg, bsizeMaxDefault);
    console.WriteLine("{0}:total amount of data to send per test run, default to {1}", dataArg, dataSizeDefault);
    console.WriteLine("{0}:minimal client thread count, default to {1}", tminArg, tminDefault);
    console.WriteLine("{0}:maximal client thread count, default to {1}", tmaxArg, tmaxDefault);
    console.WriteLine("{0}:minimal message size, default to {1}", mminArg, msizeMinDefault);
    console.WriteLine("{0}:maximal message size, default to {1}", mmaxArg, msizeMaxDefault);
    console.WriteLine("{0}:whether to queue received messages, default to {1}", qrArg, qrDefault);
    console.WriteLine("{0}:security provider, by default, all providers will be used", securityArg);

    ::ExitProcess(1);
}

void PerfTest::SetupTest(bool installCerts)
{
#ifndef PLATFORM_UNIX
    if (!remotePath.empty())
    {
        // skip auto cert installing/uninstalling until we figure out all dependencies of RingCertSetup.exe 
        //const wstring filesToCopy[] = { clientExeName, clientExeName + L".cfg", lib0, lib1, lib2, lib3, certSetupExe };
        const wstring filesToCopy[] = { clientExeName, clientExeName + L".cfg", lib0, lib1, lib2, lib3};
        for (auto const & file : filesToCopy)
        {
            console.WriteLine("{0} => {1}", file, remotePath);
            TTestUtil::CopyFile(file, Environment::GetExecutablePath(), remotePath);
        }

        if (installCerts)
        {
            console.WriteLine("certificate files => {0}", remotePath);
            TTestUtil::CopyFiles(
                Directory::GetFiles(Environment::GetExecutablePath(), L"*.pfx", false, true),
                Environment::GetExecutablePath(),
                remotePath);

            TTestUtil::CopyFiles(
                Directory::GetFiles(Environment::GetExecutablePath(), L"*.cer", false, true),
                Environment::GetExecutablePath(),
                remotePath);

            console.WriteLine("\ninstall certificate on {0}\n", computer);
            TTestUtil::CreateRemoteProcess(
                computer,
                Path::Combine(remoteWorkDir, certSetupExe),
                remoteWorkDir,
                0);

            ::Sleep(30 * 1000); //wait for completion
        }
    }
    else if (installCerts)
    {
        console.WriteLine("install certificates on local computer");

        HandleUPtr processHandle, threadHandle;
        vector<wchar_t> envBlock = vector<wchar_t>();
        
        auto error = ProcessUtility::CreateProcess(
            certSetupExe,
            wstring(),
            envBlock,
            0,
            processHandle,
            threadHandle);

        if (!error.IsSuccess())
        {
            console.WriteLine("Failed to start {0}:{1}", certSetupExe, error);
            ExitProcess(2);
        }

        WaitForSingleObject(processHandle->Value, INFINITE);
    }
#endif

    TTestUtil::DisableSendThrottling();
}

void PerfTest::CleanupTest(bool uninstallCerts)
{
#ifndef PLATFORM_UNIX

    if (uninstallCerts)
    {

        if (remotePath.empty())
        {
            console.WriteLine("uninstall certificates on local computer");

            HandleUPtr processHandle, threadHandle;
            vector<wchar_t> envBlock = vector<wchar_t>();
            
            auto error = ProcessUtility::CreateProcess(
                certSetupExe + L" r",
                wstring(),
                envBlock,
                0,
                processHandle,
                threadHandle);

            if (!error.IsSuccess())
            {
                console.WriteLine("Failed to start {0}:{1}", certSetupExe, error);
                ExitProcess(3);
            }

            WaitForSingleObject(processHandle->Value, INFINITE);
        }
        else
        {
            console.WriteLine("\nuninstall certificate on {0}", computer);
            TTestUtil::CreateRemoteProcess(
                computer,
                Path::Combine(remoteWorkDir, certSetupExe) + L" r",
                remoteWorkDir,
                0);

            ::Sleep(30 * 1000); //wait for completion
        }
    }

#endif
}
