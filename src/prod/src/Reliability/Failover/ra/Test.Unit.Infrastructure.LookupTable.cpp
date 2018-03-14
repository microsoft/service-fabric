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

enum Enum
{
    A,
    B,
    C,
    Count = 3
};

typedef LookupTableBuilder<Enum, Count> LookupTableBuilderType;
typedef LookupTable<Enum, Count> LookupTableType;

class LookupTableTest : public TestBase
{
protected:
    void TestSetup(bool defaultValue)
    {
        builder_ = make_unique<LookupTableBuilderType>(defaultValue);
    }

    void Build()
    {
        lookupTable_ = make_unique<LookupTableType>(builder_->CreateTable());
    }

    void Verify(std::initializer_list<char> expected)
    {
        Verify::AreEqual(Count * Count, expected.size(), L"Size is incorrect");
        int row = 0;
        int col = 0;
        
        for (auto const & it : expected)
        {
            wstring msg = wformatString("Element {0}. Row {1}. Column {2}", it, row, col);

            bool actual = false;
            bool didAssert = false;

            try
            {
                actual = lookupTable_->Lookup(static_cast<Enum>(row), static_cast<Enum>(col));
            }
            catch (const system_error &)
            {
                didAssert = true;
            }

            if (it == 'i')
            {
                Verify::IsTrue(didAssert, L"Did not assert. " + msg);
            }
            else 
            {
                bool expectedLookupTableValue = it == 't';
                Verify::IsTrue(!didAssert, L"Assert. " + msg);
                Verify::AreEqual(expectedLookupTableValue, actual, msg);
            }

            col++;
            if (col == 3)
            {
                col = 0;
                row++;
            }
        }
    }
    
    void BuildAndVerify(std::initializer_list<char> expected)
    {
        Build();

        Verify(expected);
    }

    unique_ptr<LookupTableType> lookupTable_;
    unique_ptr<LookupTableBuilderType> builder_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(LookupTableTestSuite, LookupTableTest)

BOOST_AUTO_TEST_CASE(InitialConstructionWorks)
{
    TestSetup(true);

    BuildAndVerify(
    {
        't', 't', 't',
        't', 't', 't',
        't', 't', 't'
    });
}

BOOST_AUTO_TEST_CASE(InitialConstructionWorksNegative)
{
    TestSetup(false);

    BuildAndVerify(
    {
        'f', 'f', 'f',
        'f', 'f', 'f',
        'f', 'f', 'f'
    });
}

BOOST_AUTO_TEST_CASE(MarkInvalid)
{
    TestSetup(false);

    builder_->MarkInvalid({ B, C });

    BuildAndVerify(
    {
        'f', 'i', 'i',
        'i', 'i', 'i',
        'i', 'i', 'i'
    });
}

BOOST_AUTO_TEST_CASE(MarkSingleValueInvalid)
{
    TestSetup(false);

    builder_->MarkInvalid({ A });

    BuildAndVerify(
    {
        'i', 'i', 'i',
        'i', 'f', 'f',
        'i', 'f', 'f'
    });
}

BOOST_AUTO_TEST_CASE(MarkTrue)
{
    TestSetup(false);

    builder_->MarkTrue(B, C);

    BuildAndVerify(
    {
        'f', 'f', 'f',
        'f', 'f', 't',
        'f', 'f', 'f'
    });
}

BOOST_AUTO_TEST_CASE(MarkTrueComplement)
{
    TestSetup(false);

    builder_->MarkTrue(C, B);

    BuildAndVerify(
    {
        'f', 'f', 'f',
        'f', 'f', 'f',
        'f', 't', 'f'
    });
}

BOOST_AUTO_TEST_CASE(MarkFalse)
{
    TestSetup(true);

    builder_->MarkFalse(B, C);

    BuildAndVerify(
    {
        't', 't', 't',
        't', 't', 'f',
        't', 't', 't'
    });
}

BOOST_AUTO_TEST_CASE(MarkFalseComplement)
{
    TestSetup(true);

    builder_->MarkFalse(C, B);

    BuildAndVerify(
    {
        't', 't', 't',
        't', 't', 't',
        't', 'f', 't'
    });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

