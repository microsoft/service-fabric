// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

// Uses ETW and logman - only works on windows for now
#if !defined(PLATFORM_UNIX)


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Common;
using namespace std;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Transport;
using namespace StateManagement;

namespace
{
    wstring const LogManPath = L"logman.exe"; //L"C:\\Windows\\System32\\logman.exe";
    wstring const LodCtrPath = L"lodctr.exe";// L"C:\\windows\\system32\\lodctr.exe";
    wstring const UnLodCtrPath = L"lodctr.exe";// UnLodCtrPath;
    const int NumberOfPartitions = 1000 * 1000;
}

class ManifestCopier
{
public:
    ManifestCopier(wstring const & workingDirectory) : workingDirectory_(workingDirectory) {}

    void CopyManifest()
    {
        TestLog::WriteInfo(L"Copying manifest");

        // Find binary
        wstring const ManifestFileName = L"Microsoft-ServiceFabric-Events.man";
        auto source = Path::GetFilePathInModuleLocation(::GetModuleHandle(NULL), ManifestFileName);
        auto target = Path::Combine(workingDirectory_, ManifestFileName);

        if (IsUpdated(source, target))
        {
            return;
        }

        auto error = File::Copy(source, target, true);
        ASSERT_IF(!error.IsSuccess(), "Failed to copy file {0}", ManifestFileName);
    }
private:

    bool IsUpdated(std::wstring const & source, std::wstring target)
    {
        if (!File::Exists(target))
        {
            return false;
        }

        DateTime sourceLastWriteTime, targetLastWriteTime;
        auto error = File::GetLastWriteTime(source, sourceLastWriteTime);
        if (!error.IsSuccess())
        {
            return false;
        }

        error = File::GetLastWriteTime(target, targetLastWriteTime);
        if (!error.IsSuccess())
        {
            return false;
        }

        auto diff = sourceLastWriteTime - targetLastWriteTime;
        return diff.TotalMilliseconds() > 0;
    }

    wstring workingDirectory_;
};

class TraceSessionUtility
{
public:
    TraceSessionUtility(wstring const & workingDirectory) : workingDirectory_(workingDirectory) { }

    void CleanupTraceSession()
    {
        ExecuteCommand(L"stop ra_test -ets", true);
    }

    void StartTraceSession()
    {
        TestLog::WriteInfo(L"Start Trace Session");

        wstring tracePath = Path::Combine(workingDirectory_, L"trace.etl");

        wstring args = L"start ra_test -p {cbd93bc2-71e5-4566-b3a7-595d8eeca6e8} -ets -bs 2048 -nb 512 1024 -o " + tracePath;

        ExecuteCommand(args, false);
    }

private:
    void ExecuteCommand(wstring const & args, bool ignoreExitCode)
    {
        processRunner_.Execute(LogManPath, args, workingDirectory_, ignoreExitCode);
    }

    ProcessRunner processRunner_;
    wstring workingDirectory_;
};

class PerfCounterUtility
{
public:
    PerfCounterUtility(wstring const & workingDirectory) : workingDirectory_(workingDirectory) {}

    void SetupSession()
    {
        TestLog::WriteInfo(L"Start Perf Session");

        for (auto const & file : Directory::GetFiles(workingDirectory_, L"*.blg", true, true))
        {
            TestLog::WriteInfo(wformatString("Deleting old perf file {0}", file));
            File::Delete(file, false);
        }

        if (File::Exists(GetCsvPath()))
        {
            TestLog::WriteInfo(wformatString("Deleting old csv {0}", GetCsvPath()));
            File::Delete(GetCsvPath());
        }

        processRunner_.Execute(LodCtrPath, wformatString("/m:{0}", GetManifestPath()), workingDirectory_, false);

        auto commandLine = L"create counter ra_perf -c \"\\Windows Fabric ESE Local Store(*)\\*\" \"\\Windows Fabric Reconfiguration Agent(*)\\*\" \"\\Windows Fabric Component JobQueue(*)\\*\" \"\\Windows Fabric Database ==> Instances(*)\\*\"  \"\\Processor(_Total)\\% Processor Time\" \"\\Event Tracing for Windows Session(ra_test)\\Events Lost\" \"\\Process(RA.test)\\*\" -f bin -si 00:00:01 -o " + GetOutputPath();

        processRunner_.Execute(LogManPath, commandLine, workingDirectory_, false);

        processRunner_.Execute(LogManPath, L"start ra_perf", workingDirectory_, false);
    }

    void CleanupSession()
    {
        processRunner_.Execute(LogManPath, L"stop ra_perf", workingDirectory_, true);

        processRunner_.Execute(LogManPath, L"delete ra_perf", workingDirectory_, true);

        processRunner_.Execute(UnLodCtrPath, wformatString("/m:{0}", GetManifestPath()), workingDirectory_, true);

        // TODO: build up the path properly instead of hardcoding it
        if (File::Exists(GetBlgFiles()) && !File::Exists(GetCsvPath()))
        {
            wstring args = wformatString("{0} -f CSV -o {1}", GetBlgFiles(), GetCsvPath());
            processRunner_.Execute(L"C:\\Windows\\System32\\relog.exe", args, workingDirectory_, false);
        }
    }

private:
    wstring GetManifestPath()
    {
        return Path::GetFilePathInModuleLocation(::GetModuleHandle(NULL), L"WFPerf.man");
    }

    wstring GetOutputPath()
    {
        return Path::Combine(workingDirectory_, L"perf.blg");
    }

    wstring GetBlgFiles()
    {
        return Path::Combine(workingDirectory_, L"perf_000001.blg");
    }

    wstring GetCsvPath()
    {
        return Path::Combine(workingDirectory_, L"perf.csv");
    }

    ProcessRunner processRunner_;
    wstring workingDirectory_;
};

class TestEnvironment
{
public:
    TestEnvironment() :
        workingDirectory_(Path::Combine(Directory::GetCurrentDirectory(), L"ratest")),
        traceUtility_(workingDirectory_),
        manifestCopier_(workingDirectory_),
        perfCounterUtility_(workingDirectory_)
    {
        Setup();
    }

    ~TestEnvironment()
    {
        Clean();
    }

private:
    void Setup()
    {
        TestLog::WriteInfo(wformatString("Setup. CWD = {0}", workingDirectory_));

        InternalClean();

        if (!Directory::Exists(workingDirectory_))
        {
            TestLog::WriteInfo(wformatString("Create {0}", workingDirectory_));
            Directory::Create(workingDirectory_);
        }

        auto raDirectory = Path::Combine(workingDirectory_, L"RA");
        if (Directory::Exists(raDirectory))
        {
            TestLog::WriteInfo(L"Deleting persisted state");
            Directory::Delete(raDirectory, true, true);
        }

        manifestCopier_.CopyManifest();
        traceUtility_.StartTraceSession();
        perfCounterUtility_.SetupSession();
    }

    void Clean()
    {
        TestLog::WriteInfo(L"Cleanup");

        InternalClean();
    }

    void InternalClean()
    {
        if (!Directory::Exists(workingDirectory_))
        {
            TestLog::WriteInfo(wformatString("Skip cleanup as {0} does not exist", workingDirectory_));
            return;
        }

        traceUtility_.CleanupTraceSession();
        perfCounterUtility_.CleanupSession();
    }

    // Needs to be the first member
    wstring workingDirectory_;
    TraceSessionUtility traceUtility_;
    ManifestCopier manifestCopier_;
    PerfCounterUtility perfCounterUtility_;
};

class TimeMeasurer
{
public:
    TimeMeasurer(wstring const & name) : name_(name)
    {
        sw_.Start();
    }

    ~TimeMeasurer()
    {
        sw_.Stop();
        TestLog::WriteInfo(wformatString("Measurement {0}: {1}s", name_, sw_.Elapsed.TotalSeconds()));
    }

private:
    Stopwatch sw_;
    wstring name_;
};

class AddInstanceRequest
{
    DENY_COPY(AddInstanceRequest);
public:
    AddInstanceRequest() {}

    AddInstanceRequest(AddInstanceRequest&& other) :
        message_(move(other.message_)),
        ipcContext_(move(other.ipcContext_))
    {
    }

    AddInstanceRequest & operator=(AddInstanceRequest&& other)
    {
        if (this != &other)
        {
            message_ = move(other.message_);
            ipcContext_ = move(other.ipcContext_);
        }

        return *this;
    }

    void Process(ReconfigurationAgent & ra)
    {
        ASSERT_IF(message_ == nullptr, "not initialized");
        ra.ProcessTransportRequest(message_, move(ipcContext_));
    }

    void Initialize()
    {
        ipcContext_ = make_unique<Federation::OneWayReceiverContext>(Federation::PartnerNodeSPtr(), Federation::NodeInstance(), Transport::MessageId());
        message_ = CreateMessage();
    }

private:
    static MessageUPtr CreateMessage()
    {
        const Epoch ccEpoch = Reader::ReadHelper<Epoch>(L"411");
        Guid partitionId = Guid::NewGuid();
        ServiceDescription sd = Default::GetInstance().SL1_SP1_STContext.SD;
        sd.Name = wformatString("fabric:/app/{0}", partitionId);

        FailoverUnitDescription ftDesc(FailoverUnitId(partitionId), ConsistencyUnitDescription(partitionId), ccEpoch);
        ReplicaDescription replicaDescription(Default::GetInstance().NodeInstance, 1, 1);
        replicaDescription.PackageVersionInstance = Default::GetInstance().SL1_SP1_STContext.ServicePackageVersionInstance;

        auto msg = RSMessage::GetAddInstance().CreateMessage(ReplicaMessageBody(ftDesc, replicaDescription, sd));
        msg->Headers.Add(ScenarioTest::GetDefaultGenerationHeader());
        return msg;
    }

    MessageUPtr message_;
    Federation::OneWayReceiverContextUPtr ipcContext_;
};

class PerformanceTest
{
protected:
    PerformanceTest()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~PerformanceTest()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    ScenarioTest & Recreate()
    {
        return holder_->Recreate();
    }

    ScenarioTest & GetScenarioTest()
    {
        return holder_->ScenarioTestObj;
    }

    std::vector<AddInstanceRequest> CreateRequests(int count);
    void ExecuteRequests(std::vector<AddInstanceRequest> & v);

    void RunCreatePhase();
    void RunServiceTypeRegisteredPhase();
    void WaitForWorkToDrain(int expected);

    ScenarioTestHolderUPtr holder_;
    unique_ptr<TestEnvironment> testEnv_;
};

bool PerformanceTest::TestSetup()
{
    testEnv_ = make_unique<TestEnvironment>();

    UnitTestContext::Option option;
    option.UseHostingPerfTestStub = true;
    option.UseHealthPerfTestStub = true;
    option.UseRealEse = true;
    option.UseIpcPerfTestStub = true;
    option.WorkingDirectory = Path::Combine(Directory::GetCurrentDirectory(), L"ratest");
    option.EtwTraceLevel = 4;
	option.FileTraceLevel = 2;

    holder_ = ScenarioTestHolder::Create(option);

    // TODO: This should ideally be removed
    GetScenarioTest().UTContext.Config.RAPMessageRetryInterval = TimeSpan::MaxValue;
    return true;
}

bool PerformanceTest::TestCleanup()
{
    holder_.reset();

    testEnv_.reset();

    return true;
}

class RequestInitializer
{
    DENY_COPY(RequestInitializer);

public:
    RequestInitializer(int index, int threadCount, vector<AddInstanceRequest> & requests) :
        requests_(requests)
    {
        startIndex_ = index * (static_cast<int>(requests.size()) / threadCount);
		endIndex_ = (index == (threadCount - 1)) ? static_cast<int>(requests.size()) : (static_cast<int>(requests.size()) / threadCount) * (index + 1);
        Common::Threadpool::Post([this]() { ThreadProc(); });
    }

    void Wait()
    {
        ev_.WaitOne();
    }

private:
    void ThreadProc()
    {
        for (int i = startIndex_; i < endIndex_; i++)
        {
            requests_[i].Initialize();
        }

        ev_.Set();
    }

    int startIndex_;
    int endIndex_;
    vector<AddInstanceRequest> & requests_;
    ManualResetEvent ev_;
};

vector<AddInstanceRequest> PerformanceTest::CreateRequests(int count)
{
    TimeMeasurer timeMeasurer(L"CreateRequests");
    vector<AddInstanceRequest> v(count);

    int threads = Environment::GetNumberOfProcessors();
    vector<unique_ptr<RequestInitializer>> requestInitializers;
    for (int i = 0; i < threads; i++)
    {
        requestInitializers.push_back(make_unique<RequestInitializer>(i, threads, v));
    }

    for (auto & it : requestInitializers)
    {
        it->Wait();
    }

    return v;
}

void PerformanceTest::ExecuteRequests(std::vector<AddInstanceRequest> & v)
{
    TimeMeasurer timeMeasurer(L"Execute");
    for (auto & it : v)
    {
        it.Process(GetScenarioTest().RA);
    }
}

void PerformanceTest::WaitForWorkToDrain(int expected)
{
    auto & perfCounters = GetScenarioTest().RA.PerfCounters;
    auto measureFunc = [&]() { return perfCounters.NumberOfCompletedJobItems.RawValue ; };

    TimeMeasurer timeMeasurer(L"Wait");

    for (int iteration = 0; iteration < 500; ++iteration)
    {
        auto start = measureFunc();
        Sleep(1000);
        auto finish = measureFunc();
        TestLog::WriteInfo(wformatString("Iteration: {0}. Start: {1}. Finish: {2}", iteration, start, finish));

        if (start == finish && start == expected)
        {
            return;
        }
    }

    Assert::CodingError("Test did not stabilize");
}

void PerformanceTest::RunCreatePhase()
{
    {
        auto requests = CreateRequests(NumberOfPartitions);
        ExecuteRequests(requests);
    }

    WaitForWorkToDrain(NumberOfPartitions);
}

void PerformanceTest::RunServiceTypeRegisteredPhase()
{
    {
        TimeMeasurer timeMeasurer(L"STR");

        GetScenarioTest().RA.ProcessServiceTypeRegistered(Default::GetInstance().SL1_SP1_STContext.CreateServiceTypeRegistration());
    }

    WaitForWorkToDrain(2 * NumberOfPartitions);
}

BOOST_AUTO_TEST_SUITE(Functional)

BOOST_FIXTURE_TEST_SUITE(PerformanceTestSuite, PerformanceTest)

// BOOST_AUTO_TEST_CASE(PerformanceTest1)
// {
// TestLog::WriteInfo(L"Starting CreatePhase");
// RunCreatePhase();

// TestLog::WriteInfo(L"Verifying");
// ASSERT_IF(GetScenarioTest().RA.PerfCounters.NumberOfCommitFailures.RawValue != 0, "Found commit failures");

// Sleep(1000);
// TestLog::WriteInfo(L"Starting ServiceTypeRegisteredPhase");
// RunServiceTypeRegisteredPhase();

// TestLog::WriteInfo(L"Verifying");
// ExecuteUnderReadLockOverMap<FailoverUnit>(
// GetScenarioTest().RA.LocalFailoverUnitMapObj,
// [](ReadOnlyLockedFailoverUnitPtr & ptr)
// {
// ASSERT_IF(!ptr, "FT cannot be null");
// auto const & ft = *ptr;
// ASSERT_IF(ft.ServiceTypeRegistration == nullptr, "{0} doesn't have str", ft);
// });
// }

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
#endif
