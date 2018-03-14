// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class ThreadpoolStub;

            class UnitTestContext;
            typedef std::unique_ptr<UnitTestContext> UnitTestContextUPtr;

            class FailoverUnitContainer;
            typedef std::unique_ptr<FailoverUnitContainer> FailoverUnitContainerUPtr;

            class ScenarioTest;
            typedef std::unique_ptr<ScenarioTest> ScenarioTestUPtr;

            class HealthSubsystemWrapperStub;

            class HostingStub;
            class RaToRapTransportStub;

            class FailoverUnitJobItemTestContext;
            typedef std::unique_ptr<FailoverUnitJobItemTestContext> FailoverUnitJobItemTestContextUPtr;

            class ScenarioTestHolder;
            typedef std::unique_ptr<ScenarioTestHolder> ScenarioTestHolderUPtr;

            class InMemoryLocalStore;

            namespace StateManagement
            {
                class ServiceTypeReadContext;
                typedef std::unique_ptr<ServiceTypeReadContext> ServiceTypeReadContextUPtr;

                class SingleFTReadContext;
                typedef std::unique_ptr<SingleFTReadContext> SingleFTReadContextUPtr;

                class StateTransferReadContext;
                typedef std::unique_ptr<StateTransferReadContext> StateTransferReadContextUPtr;

                class MultipleReadContextContainer;
                class ReadContext;
                class Reader;
                class ErrorLogger;

                namespace ReadOption
                {
                    enum Enum
                    {
                        None,
                        ShortFormat
                    };
                }
            }

            namespace StateItemHelper
            {
                class StateItemHelperBase;

                class StateItemHelperCollection;
                typedef std::unique_ptr<StateItemHelperCollection> StateItemHelperCollectionUPtr;
            }
        }
    }
}

