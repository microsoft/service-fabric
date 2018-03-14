// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;
using namespace Infrastructure;
using namespace Common;
using namespace std;

class TestLocalFailoverUnitMap
{
protected:
    TestLocalFailoverUnitMap() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestLocalFailoverUnitMap() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);
    
    struct AddEntriesResult
    {
        LocalFailoverUnitMapEntrySPtr FMEntry;
        LocalFailoverUnitMapEntrySPtr OtherFTEntry;

        vector<EntityEntryBase*> CreateExpectation(bool fmEntry, bool otherEntry) const
        {
            vector<EntityEntryBase*> rv;

            if (fmEntry)
            {
                rv.push_back(FMEntry.get());
            }

            if (otherEntry)
            {
                rv.push_back(OtherFTEntry.get());
            }

            return rv;
        }
    };

    class EntryState
    {
	public:
        enum Enum
        {
            NotExisting = 0,
            NullFT = 1,
            HasFTInitial = 2,
            HasFTUpdated = 3,
            Deleted = 4,
        };
    };

    class PersistenceResult
    {
	public:
        enum Enum
        {
            Success,
            Failure
        };
    };

    AddEntriesResult AddEntries(bool addFMEntry, bool addOtherEntry);
    void ExecuteGetAllAndVerifyResult(bool excludeFM, bool fmExpected, bool otherExpected, AddEntriesResult const & addEntryResult);
    void ExecuteGetFMEntriesAndVerifyResult(bool expected, AddEntriesResult const & addEntryResult);

    LocalFailoverUnitMapEntrySPtr GetOrCreate(bool createFlag);
    LocalFailoverUnitMapEntrySPtr GetOrCreate(bool createFlag, std::wstring const & ftShortName);

    InMemoryLocalStore & GetStore();

    static FailoverUnitId GetFTId();
    static FailoverUnitId GetFTId(std::wstring const & ftShortName);
    static wstring GetFTShortName();
        
    UnitTestContextUPtr utContext_;
};


bool TestLocalFailoverUnitMap::TestSetup()
{
	utContext_ = UnitTestContext::Create(UnitTestContext::Option::None);
	return true;
}

bool TestLocalFailoverUnitMap::TestCleanup()
{
	return utContext_->Cleanup();
}

TestLocalFailoverUnitMap::AddEntriesResult TestLocalFailoverUnitMap::AddEntries(bool addFMEntry, bool addOtherEntry)
{
    AddEntriesResult result;

    if (addFMEntry)
    {
        result.FMEntry = GetOrCreate(true, L"FM");
    }

    if (addOtherEntry)
    {
        result.OtherFTEntry = GetOrCreate(true, L"SP1");
    }

    return result;
}

template<typename T>
vector<T*> ConvertVectorOfSharedPtrToVectorOfPtr(vector<std::shared_ptr<T>> const & result)
{
    vector<T*> rv;

    for (auto const & it : result)
    {
        rv.push_back(it.get());
    }

    return rv;
}

void TestLocalFailoverUnitMap::ExecuteGetAllAndVerifyResult(bool excludeFM, bool fmExpected, bool otherExpected, AddEntriesResult const & addEntryResult)
{
    auto expectedEntities = addEntryResult.CreateExpectation(fmExpected, otherExpected);

    auto entryResultSPtr = utContext_->LFUM.GetAllFailoverUnitEntries(excludeFM);

    // Do this to not define write to text writer for entity entry
    auto entryResult = ConvertVectorOfSharedPtrToVectorOfPtr(entryResultSPtr);
    
    Verify::Vector<EntityEntryBase*>(entryResult, entryResult);
}

void TestLocalFailoverUnitMap::ExecuteGetFMEntriesAndVerifyResult(bool expected, AddEntriesResult const & addEntryResult)
{
    vector<EntityEntryBase*> expectedEntities = addEntryResult.CreateExpectation(expected, false);

    auto result = utContext_->LFUM.GetFMFailoverUnitEntries();
    auto casted = ConvertVectorOfSharedPtrToVectorOfPtr(result);

    Verify::Vector(casted, expectedEntities);
}

FailoverUnitId TestLocalFailoverUnitMap::GetFTId()
{
    return GetFTId(GetFTShortName());
}

FailoverUnitId TestLocalFailoverUnitMap::GetFTId(std::wstring const & shortName)
{
    return StateManagement::Default::GetInstance().LookupFTContext(shortName).FUID;
}
    
wstring TestLocalFailoverUnitMap::GetFTShortName()
{
    return L"SP1";
}

LocalFailoverUnitMapEntrySPtr TestLocalFailoverUnitMap::GetOrCreate(bool createFlag)
{
    return GetOrCreate(createFlag, GetFTShortName());
}

LocalFailoverUnitMapEntrySPtr TestLocalFailoverUnitMap::GetOrCreate(bool createFlag, std::wstring const & ftShortName)
{
    return dynamic_pointer_cast<LocalFailoverUnitMapEntry>(utContext_->LFUM.GetOrCreateEntityMapEntry(GetFTId(ftShortName), createFlag));
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestLocalFailoverUnitMapSuite,TestLocalFailoverUnitMap)

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMTrue_OnlyFMEntries)
{
    auto addResult = AddEntries(true, false);

    ExecuteGetAllAndVerifyResult(true, false, false, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMTrue_NoEntries)
{
    auto addResult = AddEntries(false, false);

    ExecuteGetAllAndVerifyResult(true, false, false, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMTrue_BothEntries)
{
    auto addResult = AddEntries(true, true);

    ExecuteGetAllAndVerifyResult(true, false, true, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMTrue_OnlyNonFMEntries)
{
    auto addResult = AddEntries(false, true);

    ExecuteGetAllAndVerifyResult(true, false, true, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMFalse_OnlyFMEntries)
{
    auto addResult = AddEntries(true, false);

    ExecuteGetAllAndVerifyResult(false, true, false, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMFalse_NoEntries)
{
    auto addResult = AddEntries(false, false);

    ExecuteGetAllAndVerifyResult(false, false, false, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMFalse_BothEntries)
{
    auto addResult = AddEntries(true, true);

    ExecuteGetAllAndVerifyResult(false, true, true, addResult);
}

BOOST_AUTO_TEST_CASE(GetAllTest_ExcludeFMFalse_OnlyNonFMEntries)
{
    auto addResult = AddEntries(false, true);

    ExecuteGetAllAndVerifyResult(false, false, true, addResult);
}

BOOST_AUTO_TEST_CASE(GetFMEntries_OnlyFM)
{
    auto addResult = AddEntries(true, false);

    ExecuteGetFMEntriesAndVerifyResult(true, addResult);
}

BOOST_AUTO_TEST_CASE(GetFMEntries_Both)
{
    auto addResult = AddEntries(true, true);

    ExecuteGetFMEntriesAndVerifyResult(true, addResult);
}

BOOST_AUTO_TEST_CASE(GetFMEntries_None)
{
    auto addResult = AddEntries(false, false);

    ExecuteGetFMEntriesAndVerifyResult(false, addResult);
}

BOOST_AUTO_TEST_CASE(GetFMEntries_OnlyOtherEntries)
{
    auto addResult = AddEntries(false, true);

    ExecuteGetFMEntriesAndVerifyResult(false, addResult);
}

BOOST_AUTO_TEST_CASE(GetByOwner_Fmm)
{
    auto addResult = AddEntries(true, true);

    auto entries = utContext_->LFUM.GetFailoverUnitEntries(*FailoverManagerId::Fmm);
    auto casted = ConvertVectorOfSharedPtrToVectorOfPtr(entries);

    auto expected = addResult.CreateExpectation(true, false);
    Verify::Vector(expected, casted);
}

BOOST_AUTO_TEST_CASE(GetByOwner_Fm)
{
    auto addResult = AddEntries(true, true);

    auto entries = utContext_->LFUM.GetFailoverUnitEntries(*FailoverManagerId::Fm);
    auto casted = ConvertVectorOfSharedPtrToVectorOfPtr(entries);

    auto expected = addResult.CreateExpectation(false, true);
    Verify::Vector(expected, casted);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
