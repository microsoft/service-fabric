// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            // A class that encapsulates everything required to write a unit test for the RA
            // Supports different types of stubs for external components
            // Also provides a context for storing pieces of data to be associated with the RA itself and can be accessed from the root
            class UnitTestContext
            {
                DENY_COPY(UnitTestContext);

            public:
                class Option
                {
                public:
                    enum Enum
                    {
                        None = 0x00,
                        StubJobQueueManager = 0x01,
                        TestAssertEnabled = 0x02
                    };

                    Option() : 
                        UseStubJobQueueManager(false),
                        EnableTestAssert(false),
                        UseHostingPerfTestStub(false),
                        UseHealthPerfTestStub(false),
                        UseRealEse(false),
                        UseIpcPerfTestStub(false),
                        WorkingDirectory(L"."),
                        ConsoleTraceLevel(3),
                        EtwTraceLevel(2),
						FileTraceLevel(2)
                    {
                    }

                    Option(Enum options)
                    {
                        UseStubJobQueueManager = (options & StubJobQueueManager) != 0;
                        EnableTestAssert = (options & TestAssertEnabled) != 0;
                        UseHostingPerfTestStub = false;
                        UseHealthPerfTestStub = false;
                        UseRealEse = false;
                        WorkingDirectory = L".";
                        ConsoleTraceLevel = 3;
                        EtwTraceLevel = 2;
						FileTraceLevel = 2;
                        UseIpcPerfTestStub = false;
                    }

                    std::wstring WorkingDirectory;

                    // Indicates whether to use in memory impl of ESE or real ESE
                    bool UseRealEse;

                    // Indicates whether to use the real threadpool or fake threadpool
                    bool UseStubJobQueueManager;

                    bool EnableTestAssert;

                    // Indicates whether to use real hosting stub which allows for complete control
                    // Or the perf test stub which does not track any API
                    bool UseHostingPerfTestStub;

                    // Indicates whether to use real hosting stub which allows for verification
                    // Or the perf test stub which does not track anything
                    bool UseHealthPerfTestStub;

                    // Indicates whether to use real ipc stub which allows for verification by putting message in a list
                    // Or the perf test stub which does not track anything
                    bool UseIpcPerfTestStub;

                    int ConsoleTraceLevel;

                    int EtwTraceLevel;
					
					int FileTraceLevel;
                };

                UnitTestContext(Option options);

                __declspec(property(get = get_HasStubJobQueueManager)) bool HasStubJobQueueManager;
                bool get_HasStubJobQueueManager() const { return options_.UseStubJobQueueManager; }

                __declspec(property(get = get_ThreadpoolStubObj)) ThreadpoolStub & ThreadpoolStubObj;
                ThreadpoolStub & get_ThreadpoolStubObj() 
                { 
                    ASSERT_IF(!HasStubJobQueueManager, "Must have stub jqm");
                    return *threadpoolStub_; 
                }

                __declspec(property(get = get_ContextMap)) TestContextMap & ContextMap;
                TestContextMap & get_ContextMap() { return contextMap_; }

                __declspec(property(get = get_ProcessTerminationServiceStubObj)) ProcessTerminationServiceStub & ProcessTerminationServiceStubObj;
                ProcessTerminationServiceStub & get_ProcessTerminationServiceStubObj() { return processTerminationServiceStub_; }

                __declspec(property(get = get_RA)) ReconfigurationAgent & RA;
                ReconfigurationAgent & get_RA() { return *ra_; }

                __declspec(property(get = get_LFUM)) Infrastructure::LocalFailoverUnitMap & LFUM;
                Infrastructure::LocalFailoverUnitMap & get_LFUM() { return RA.LocalFailoverUnitMapObj; }

                __declspec(property(get = get_Reliability)) ReliabilitySubsystemStub & Reliability;
                ReliabilitySubsystemStub & get_Reliability() const { return *reliability_; }

                __declspec(property(get = get_NodeConfig)) Common::FabricNodeConfig & NodeConfig;
                Common::FabricNodeConfig & get_NodeConfig() const { return *nodeConfig_; }

                __declspec(property(get = get_FaultInjectedLfumStore)) Storage::FaultInjectionAdapter & FaultInjectedLfumStore;
                Storage::FaultInjectionAdapter & get_FaultInjectedLfumStore() { return dynamic_cast<Storage::FaultInjectionAdapter&>(*ra_->LfumStore); }

                __declspec(property(get = get_Config)) Reliability::FailoverConfig & Config;
                Reliability::FailoverConfig & get_Config() { return configContainer_->Config; }

                __declspec(property(get = get_Hosting)) HostingStub & Hosting;
                HostingStub& get_Hosting()
                {
                    auto casted = dynamic_cast<HostingStub*>(hostingStub_.get());
                    ASSERT_IF(casted == nullptr || options_.UseHostingPerfTestStub, "Trying to get hosting stub when not supported");
                    return *casted;
                }

                __declspec(property(get = get_Options)) Option const & Options;
                Option const & get_Options() const { return options_; }

                __declspec(property(get = get_FederationWrapper)) FederationWrapperStub & FederationWrapper;
                FederationWrapperStub & get_FederationWrapper() { return *federationWrapper_; }

                __declspec(property(get = get_RapTransport)) RaToRapTransportStub & RapTransport;
                RaToRapTransportStub & get_RapTransport() { return *rapTransport_; }

                __declspec(property(get = get_Clock)) TestClock & Clock;
                TestClock & get_Clock() const { return *clock_; }

                __declspec(property(get = get_IClockSPtr)) Infrastructure::IClockSPtr IClockSPtr;
                Infrastructure::IClockSPtr get_IClockSPtr() const { return std::static_pointer_cast<Infrastructure::IClock>(clock_); }

                static UnitTestContextUPtr CreateWithOpenRA()
                {
                    return Create();
                }

                static UnitTestContextUPtr Create()
                {
                    return Create(Option::None);
                }

                static UnitTestContextUPtr Create(int options)
                {
                    return Create(static_cast<Option::Enum>(options));
                }

                static UnitTestContextUPtr Create(Option::Enum options)
                {
                    return Create(Option(options));
                }

                static UnitTestContextUPtr Create(Option options)
                {
                    auto context = Common::make_unique<UnitTestContext>(options);
                    bool opened = context->Setup();
                    Verify::IsTrueLogOnError(opened, L"Failed to open RA Unit test context with Open RA");

                    return context;
                }

                // Retrieve the ut context given the RA
                static UnitTestContext & GetUnitTestContext(ReconfigurationAgent & ra)
                {
                    auto & root = const_cast<TestComponentRoot &>(dynamic_cast<TestComponentRoot const &>(ra.Root));
                    return root.UTContext;
                }

                void UpdateThreadpool(Infrastructure::IThreadpoolUPtr && newThreadpool);
                bool Setup();
                bool Cleanup();

                void DrainAllQueues();

                void Close();
                                
                std::shared_ptr<NodeUpOperationFactory> RestartRA();

            private:

                void InitializeNodeConfig();
                void InitializeAssertConfig();
                void InitializeTraceConfig();

                std::shared_ptr<NodeUpOperationFactory> OpenRA();

                Storage::KeyValueStoreFactory::Parameters CreateStoreFactoryParameters();

                std::shared_ptr<TestComponentRoot> root_;
                Option options_;
                bool isOpen_;

                ReliabilitySubsystemStub* reliability_;
                std::unique_ptr<FederationWrapperStub> federationWrapper_;
                std::unique_ptr<ReconfigurationAgent> ra_;

                RaToRapTransportStub* rapTransport_;
                std::unique_ptr<Hosting2::IHostingSubsystem> hostingStub_;
                Common::FabricNodeConfigSPtr nodeConfig_;
                Common::FileConfigStoreSPtr nodeConfigStore_;

                std::shared_ptr<TestClock> clock_;

                std::unique_ptr<FailoverConfigContainer> configContainer_;
                ThreadpoolStub * threadpoolStub_;
                TestContextMap contextMap_;
                ProcessTerminationServiceStub processTerminationServiceStub_;

                Storage::InMemoryKeyValueStoreStateSPtr lfumStoreState_;
            };
        }
    }
}
