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
using namespace Diagnostics;

class InitializationThreadpool : public Infrastructure::IThreadpool
{
    DENY_COPY(InitializationThreadpool);
public:
    InitializationThreadpool(ReconfigurationAgent & root) :
        ra_(root)
    {
    }

    void ExecuteOnThreadpool(Common::ThreadpoolCallback callback) override
    {
        Threadpool::Post(callback);
    }

    bool EnqueueIntoMessageQueue(MessageProcessingJobItem<ReconfigurationAgent> &) override
    {
        Assert::CodingError("EnqueueIntoMessageQueue not expected during initialization of RA");
    }

    Common::AsyncOperationSPtr BeginScheduleEntity(
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent) override
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    Common::ErrorCode EndScheduleEntity(
        Common::AsyncOperationSPtr const & op) override
    {
        return CompletedAsyncOperation::End(op);
    }

    Common::AsyncOperationSPtr BeginScheduleCommitCallback(
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent) override
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    Common::ErrorCode EndScheduleCommitCallback(
        Common::AsyncOperationSPtr const & op) override
    {
        return CompletedAsyncOperation::End(op);
    }

    void Close() override
    {
    }

    static Infrastructure::IThreadpoolUPtr Factory(ReconfigurationAgent & ra)
    {
        return Common::make_unique<InitializationThreadpool>(ra);
    }

private:
    ReconfigurationAgent & ra_;
};

UnitTestContext::UnitTestContext(Option options) :
    root_(make_shared<TestComponentRoot>(*this)),
    options_(options),
    clock_(make_shared<TestClock>()),
    threadpoolStub_(nullptr),
    isOpen_(false),
    lfumStoreState_(make_shared<Storage::InMemoryKeyValueStoreState>())
{
}

bool UnitTestContext::Setup()
{   
    // Trace is the first thing that should be initialized
    InitializeTraceConfig();

    InitializeNodeConfig();

    configContainer_ = make_unique<FailoverConfigContainer>();
    configContainer_->Config.GracefulReplicaShutdownMaxDurationEntry.Test_SetValue(TimeSpan::Zero); // in general tests do not need to wait here

    // Assert config needs to be initialized once again after all other config has been initialized
    // Otherwise other config init resets assert config
    InitializeAssertConfig();

    federationWrapper_ = make_unique<FederationWrapperStub>(StateManagement::Default::GetInstance().NodeInstance);
    
    reliability_ = new ReliabilitySubsystemStub(nodeConfig_, options_.WorkingDirectory);
    IReliabilitySubsystemWrapperSPtr reliability(dynamic_cast<IReliabilitySubsystemWrapper*>(reliability_));

    if (options_.UseHostingPerfTestStub)
    {
        hostingStub_ = make_unique<PerfTestHostingStub>();
    }
    else
    {
        hostingStub_ = make_unique<HostingStub>();
    }

    rapTransport_ = new RaToRapTransportStub(!options_.UseIpcPerfTestStub);
    IRaToRapTransportUPtr rapTransportUPtr;
    rapTransportUPtr.reset(rapTransport_);

    Health::HealthSubsystemWrapperUPtr healthUPtr;
    if (options_.UseHealthPerfTestStub)
    {
        healthUPtr = make_unique<PerfTestHealthSubsystemWrapperStub>();
    }
    else
    {
        healthUPtr = make_unique<HealthSubsystemWrapperStub>();
    }
    
    /*
        There is some special logic here with respect to the threadpools
        The code needs to support closing and opening the RA
        The RA gets a pointer to the threadpool factory and uses that to create the threadpool
        When the unit tests want to control the threading explicitly they ask for a stub job queue manager
        which is the test threadpool which does not execute any work unless explicitly requested

        This is fine for all cases once the RA is open because the RA is completely async
        EXCEPT for the Open code path when it loads the LFUM which has a WaitOne (blocking)
        because Open is sync

        Thus, if the test has requested the RA to be opened with a stub threadpool then
        open it with a threadpool implementation that completes all work synchronously 

        Once the RA open is complete replace the threadpool with the test threadpool
     */

    ReconfigurationAgentConstructorParameters params = {0};
    params.Root = root_.get();
    params.ReliabilityWrapper = reliability;
    params.FederationWrapper = federationWrapper_.get();
    params.IpcServer = nullptr;
    params.Hosting = hostingStub_.get();
    params.Clock = clock_;
    params.ProcessTerminationService = processTerminationServiceStub_.ProcessTerminationServiceObj;

    unique_ptr<IEventWriter> eventWriterUPtr = make_unique<EventWriterStub>();

    ra_ = make_unique<ReconfigurationAgent>(
        params,
        move(rapTransportUPtr),
        CreateStoreFactoryParameters(),
        move(healthUPtr),
        configContainer_->Config,
        make_shared<FailoverUnitPreCommitValidator>(),
        options_.UseStubJobQueueManager ? &InitializationThreadpool::Factory : &Infrastructure::Threadpool::Factory,
        move(eventWriterUPtr));

    OpenRA();

    return true;
}

shared_ptr<NodeUpOperationFactory> UnitTestContext::RestartRA()
{
    Close();

    FederationWrapper.IncrementInstanceId();

    return OpenRA();
}

shared_ptr<NodeUpOperationFactory> UnitTestContext::OpenRA()
{
    shared_ptr<NodeUpOperationFactory> rv = make_shared<NodeUpOperationFactory>();
    ErrorCode error = ra_->Open(*rv);

    Verify::IsTrueLogOnError(error.IsSuccess(), wformatString("Failed to open ra {0}", error));
    isOpen_ = true;

    if (options_.UseStubJobQueueManager)
    {
        threadpoolStub_ = new ThreadpoolStub(*ra_, true);
        Infrastructure::IThreadpoolUPtr ptr(threadpoolStub_);
        ra_->Test_SetThreadpool(move(ptr));
    }

    return rv;
}

void UnitTestContext::UpdateThreadpool(Infrastructure::IThreadpoolUPtr && newThreadpool)
{
    ASSERT_IF(!HasStubJobQueueManager, "Must hae stub jqm");
    threadpoolStub_ = dynamic_cast<ThreadpoolStub*>(newThreadpool.get());
    ASSERT_IF(threadpoolStub_ == nullptr, "Can't be null");
    ra_->Test_SetThreadpool(move(newThreadpool));
}

bool UnitTestContext::Cleanup()
{
    Close();
    ra_.reset();

    rapTransport_ = nullptr;
    hostingStub_ = nullptr;
    reliability_ = nullptr;

    return true;
}

void UnitTestContext::DrainAllQueues()
{
    ThreadpoolStubObj.DrainAll(RA);
}

void UnitTestContext::Close()
{
    if (isOpen_)
    {
        isOpen_ = false;
        ra_->Close();
    }
}

namespace
{
    template<typename T>
    shared_ptr<T> CreateConfiguration(std::wstring const & string)
    {
        auto store = make_shared<FileConfigStore>(string, true);
        return make_shared<T>(store);
    }
}

void UnitTestContext::InitializeNodeConfig()
{
    wstring nodeConfig = wformatString("[FabricNode]\r\nNodeVersion={0}\r\n", Default::GetInstance().NodeVersionInstance);
    nodeConfigStore_ = make_shared<FileConfigStore>(nodeConfig, true);
    nodeConfig_ = make_shared<FabricNodeConfig>(nodeConfigStore_);
}

void UnitTestContext::InitializeTraceConfig()
{
    ConfigurationManager().InitializeTraceConfig(options_.ConsoleTraceLevel, options_.FileTraceLevel, options_.EtwTraceLevel);
}

void UnitTestContext::InitializeAssertConfig()
{
    ConfigurationManager().InitializeAssertConfig(options_.EnableTestAssert != 0);
}

Storage::KeyValueStoreFactory::Parameters UnitTestContext::CreateStoreFactoryParameters()
{
    Storage::KeyValueStoreFactory::Parameters parameters;
    parameters.IsInMemory = !options_.UseRealEse;
    parameters.IsFaultInjectionEnabled = true;

    if (options_.UseRealEse)
    {
        parameters.StoreFactory = make_shared<Store::StoreFactory>();

        wstring eseFolder(L"RAStoreTest");
        bool exists = Directory::Exists(eseFolder);
        TestLog::WriteInfo(wformatString("Path {0}. Exists {1}", eseFolder, exists));

        if (exists)
        {
            auto error = Directory::Delete(eseFolder, true, true);
            ASSERT_IF(!error.IsSuccess(), "Failed to clean {0} {1}", eseFolder, error);
        }

        Directory::Create(eseFolder);

        parameters.WorkingDirectory = eseFolder;
    }
    else
    {
        parameters.InMemoryKeyValueStoreState = lfumStoreState_;
    }

    return parameters;
}
