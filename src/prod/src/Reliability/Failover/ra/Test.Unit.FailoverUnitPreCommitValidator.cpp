// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestFailoverUnitPreCommitValidator : public TestBase
{
protected:
    TestFailoverUnitPreCommitValidator()
    {
        utContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled);
    }

    ~TestFailoverUnitPreCommitValidator()
    {
        utContext_->Cleanup();
    }

    UnitTestContextUPtr utContext_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFailoverUnitPreCommitValidatorSuite, TestFailoverUnitPreCommitValidator)

BOOST_AUTO_TEST_CASE(SanityTest)
{    
    auto ftContext = Default::GetInstance().SP1_FTContext;
    ftContext.RA = &utContext_->RA;
    auto store = make_shared<Storage::InMemoryKeyValueStore>();
    
    auto old = Reader::ReadHelper<FailoverUnitSPtr>(L"C None 000/000/411 1:1 -", ftContext);
    auto entry = make_shared<EntityEntry<FailoverUnit>>(ftContext.FUID, store, move(old));

    auto current = Reader::ReadHelper<FailoverUnitSPtr>(L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]", ftContext);

    CommitDescription<FailoverUnit> description;
    description.Type.StoreOperationType = Storage::Api::OperationType::Update;
    description.Type.IsInMemoryOperation = true;
    description.Data = current;

    FailoverUnitPreCommitValidator validator;

    Verify::Asserts([&]() { validator.OnPreCommit(entry, description); }, L"pre commit assert");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
