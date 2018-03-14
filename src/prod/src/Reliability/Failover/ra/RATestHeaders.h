// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define BOOST_TEST_ALTERNATIVE_INIT_API

#ifndef PLATFORM_UNIX
#define BOOST_TEST_NO_MAIN
#endif

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "RATestPointers.h"
#include "Test.Utility.TestLog.h"
#include "Test.Utility.TestComponentRoot.h"
#include "Test.Utility.ContextMap.h"
#include "Test.Utility.Clock.h"
#include "Test.Utility.AsyncApiStub.h"
#include "Test.Utility.ProcessTerminationServiceStub.h"
#include "Test.Utility.Threadpool.h"
#include "Test.Utility.FailoverConfigContainer.h"
#include "Test.Utility.StateReaderCore.h"
#include "Test.Utility.Verify.h"
#include "Test.Utility.CommitTypeDescriptionUtility.h"
#include "Test.Utility.StubUpdateContext.h"
#include "Test.Utility.EventWriterStub.h"
#include "Test.Utility.RAToRApTransportStub.h"
#include "Test.Utility.FederationWrapperStub.h"
#include "Test.Utility.ReliabilitySubsystemStub.h"
#include "Test.Utility.StateReaderCore.h"
#include "Test.Utility.h"
#include "Test.Utility.StateReader.h"
#include "Test.Utility.StateDefaults.h"
#include "Test.Utility.ReplicaStoreHolder.h"
#include "Test.Utility.StateReaderImpl.h"
#include "Test.Utility.HostingStub.h"
#include "Test.Utility.FailoverUnitContainer.h"
#include "Test.Utility.ConfigurationManager.h"
#include "Test.Utility.UnitTestContext.h"
#include "Test.Utility.HealthSubsystemWrapper.h"
#include "Test.Utility.StateItemHelper.h"
#include "Test.Utility.EntityExecutionContextTestUtility.h"
#include "Test.Utility.ScenarioTest.h"
#include "Test.Utility.ScenarioTestHolder.h"
#include "Test.Utility.TestBase.h"
#include "Test.Utility.StateMachineTestBase.h"
#include "Test.Utility.Upgrade.h"
#include "Test.Infrastructure.TestEntity.h"
#include "Test.Infrastructure.Utility.h"
#include "Test.Utility.FMMessageStateHolder.h"
