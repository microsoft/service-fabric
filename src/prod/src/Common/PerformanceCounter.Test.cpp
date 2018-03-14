// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Common/PerformanceCounter.h"
#include "Common/PerformanceCounterSetInstanceType.h"
#include "Common/PerformanceCounterType.h"
#include "Common/PerformanceCounterData.h"
#include "Common/PerformanceProvider.h"
#include "Common/PerformanceProviderCollection.h"
#include "Common/PerformanceCounterDefinition.h"
#include "Common/PerformanceCounterSetInstance.h"
#include "Common/PerformanceCounterSet.h"
#include "Common/PerformanceCounterSetDefinition.h"
#include "Common/PerformanceProviderDefinition.h"

namespace Common
{
    Guid testCounterSet1Id(L"B11EA2D6-F1E0-4C4B-9C26-F304AC4B1462");

    class TestCounterSet1
    {
        DENY_COPY(TestCounterSet1)

        BEGIN_COUNTER_SET_DEFINITION(L"B11EA2D6-F1E0-4C4B-9C26-F304AC4B1462", L"B11EA2D6-F1E0-4C4B-9C26-F304AC4B1462", L"B11EA2D6-F1E0-4C4B-9C26-F304AC4B1462", PerformanceCounterSetInstanceType::Multiple)
            COUNTER_DEFINITION(1, PerformanceCounterType::RawBase64, L"counter1", L"counter1 description")
            COUNTER_DEFINITION(2, PerformanceCounterType::AverageBase, L"counter2", L"", noDisplay)
            COUNTER_DEFINITION_WITH_BASE(3, 2, PerformanceCounterType::AverageTimer32, L"counter3", L"counter3 description")
        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(Count)
        DECLARE_COUNTER_INSTANCE(TimerBase)
        DECLARE_COUNTER_INSTANCE(Timer)

        BEGIN_COUNTER_SET_INSTANCE(TestCounterSet1)
            DEFINE_COUNTER_INSTANCE(Count, 1)
            DEFINE_COUNTER_INSTANCE(TimerBase, 2)
            DEFINE_COUNTER_INSTANCE(Timer, 3)
        END_COUNTER_SET_INSTANCE()

    public:

        PerformanceCounterSetInstance & Instance()
        {
            return *instance_;
        }
    };

    INITIALIZE_COUNTER_SET(TestCounterSet1)

    class PerformanceCounterTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(PerformanceCounterTestSuite,PerformanceCounterTest)

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterData)
    {
        PerformanceCounterData data = { 0 };

        data.Increment();

        VERIFY_ARE_EQUAL(1, data.Value);
        VERIFY_ARE_EQUAL(data.Value, data.RawValue);

        data.IncrementBy(8999);

        VERIFY_ARE_EQUAL(9000, data.Value);
        VERIFY_ARE_EQUAL(data.Value, data.RawValue);

        data.Decrement();

        VERIFY_ARE_EQUAL(8999, data.Value);
        VERIFY_ARE_EQUAL(data.Value, data.RawValue);

        data.Value = (LONGLONG)0xFF << 32;
        
        VERIFY_ARE_EQUAL((LONGLONG)0xFF << 32, data.Value);
        VERIFY_ARE_EQUAL(data.Value, data.RawValue);

        data.Increment();

        VERIFY_ARE_EQUAL(((LONGLONG)0xFF << 32) + 1, data.Value);
        VERIFY_ARE_EQUAL(data.Value, data.RawValue);
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceProviderCollection)
    {
        Guid id1 = Guid::NewGuid();
        Guid id2 = Guid::NewGuid();

        // initial lookup, creates provider
        auto sptr1 = PerformanceProviderCollection::GetOrCreateProvider(id1);
        VERIFY_IS_NOT_NULL(sptr1);
        VERIFY_IS_NOT_NULL(sptr1->Handle());
        auto wptr1 = std::weak_ptr<PerformanceProvider>(sptr1);

        auto sptr2 = PerformanceProviderCollection::GetOrCreateProvider(id2);
        VERIFY_IS_NOT_NULL(sptr2);
        VERIFY_IS_NOT_NULL(sptr2->Handle());
        auto wptr2 = std::weak_ptr<PerformanceProvider>(sptr2);

        // providers should be different
        VERIFY_ARE_NOT_EQUAL(sptr1, sptr2);
        VERIFY_ARE_NOT_EQUAL(sptr1->Handle(), sptr2->Handle());

        // release of existing provider should destroy instance
        sptr1.reset();

        VERIFY_IS_TRUE(wptr1.expired());
        VERIFY_IS_FALSE(wptr2.expired());

        // GetOrCreateProvider should recreate instance
        sptr1 = PerformanceProviderCollection::GetOrCreateProvider(id1);
        VERIFY_IS_NOT_NULL(sptr1);

        // lookup of existing providers should always return the same
        auto sptr11 = PerformanceProviderCollection::GetOrCreateProvider(id1);
        auto sptr12 = PerformanceProviderCollection::GetOrCreateProvider(id1);
        auto sptr13 = PerformanceProviderCollection::GetOrCreateProvider(id1);

        VERIFY_ARE_EQUAL(sptr1, sptr11);
        VERIFY_ARE_EQUAL(sptr11, sptr12);
        VERIFY_ARE_EQUAL(sptr13, sptr13);
        
        auto sptr21 = PerformanceProviderCollection::GetOrCreateProvider(id2);
        auto sptr22 = PerformanceProviderCollection::GetOrCreateProvider(id2);
        auto sptr23 = PerformanceProviderCollection::GetOrCreateProvider(id2);

        VERIFY_ARE_EQUAL(sptr2, sptr21);
        VERIFY_ARE_EQUAL(sptr21, sptr22);
        VERIFY_ARE_EQUAL(sptr23, sptr23);
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterSet)
    {
        Guid providerId = Guid::NewGuid();
        Guid counterSetId = Guid::NewGuid();

        // create counter set
        auto counterSet = std::make_shared<PerformanceCounterSet>(providerId, counterSetId, PerformanceCounterSetInstanceType::Multiple);

        VERIFY_ARE_EQUAL(counterSetId, counterSet->CounterSetId);

        // check the counterset create the provider
        auto provider = PerformanceProviderCollection::GetOrCreateProvider(providerId);
        auto providerWeak = std::weak_ptr<PerformanceProvider>(provider);

        VERIFY_ARE_EQUAL(provider->Handle(), counterSet->ProviderHandle);

        // check that counterset holds reference to provider
        provider.reset();
        VERIFY_IS_FALSE(providerWeak.expired());

        // check that counters are registered correctly
        std::vector<PerformanceCounterType::Enum> types;
        types.push_back(PerformanceCounterType::QueueLength);
        types.push_back(PerformanceCounterType::LargeQueueLength);
        types.push_back(PerformanceCounterType::QueueLength100Ns );
        types.push_back(PerformanceCounterType::QueueLengthObjectTime);
        types.push_back(PerformanceCounterType::RawData32);
        types.push_back(PerformanceCounterType::RawData64);
        types.push_back(PerformanceCounterType::RawDataHex32);
        types.push_back(PerformanceCounterType::RawDataHex64);
        types.push_back(PerformanceCounterType::RateOfCountPerSecond32);
        types.push_back(PerformanceCounterType::RateOfCountPerSecond64);
        types.push_back(PerformanceCounterType::RawFraction32);
        types.push_back(PerformanceCounterType::RawFraction64);
        types.push_back(PerformanceCounterType::RawBase32);
        types.push_back(PerformanceCounterType::RawBase64);
        types.push_back(PerformanceCounterType::SampleFraction);
        types.push_back(PerformanceCounterType::SampleCounter);
        types.push_back(PerformanceCounterType::SampleBase);
        types.push_back(PerformanceCounterType::AverageTimer32);
        types.push_back(PerformanceCounterType::AverageBase);
        types.push_back(PerformanceCounterType::AverageCount64);
        types.push_back(PerformanceCounterType::PercentageActive);
        types.push_back(PerformanceCounterType::PercentageNotActive);
        types.push_back(PerformanceCounterType::PercentageActive100Ns);
        types.push_back(PerformanceCounterType::PercentageNotActive100Ns);
        types.push_back(PerformanceCounterType::ElapsedTime);
        types.push_back(PerformanceCounterType::MultiTimerPercentageActive);
        types.push_back(PerformanceCounterType::MultiTimerPercentageNotActive);
        types.push_back(PerformanceCounterType::MultiTimerPercentageActive100Ns);
        types.push_back(PerformanceCounterType::MultiTimerPercentageNotActive100Ns);
        types.push_back(PerformanceCounterType::MultiTimerBase);
        types.push_back(PerformanceCounterType::Delta32);
        types.push_back(PerformanceCounterType::Delta64);
        types.push_back(PerformanceCounterType::ObjectSpecificTimer);
        types.push_back(PerformanceCounterType::PrecisionSystemTimer);
        types.push_back(PerformanceCounterType::PrecisionTimer100Ns);
        types.push_back(PerformanceCounterType::PrecisionObjectSpecificTimer);

        for (size_t i = 0; i < types.size(); ++i)
        {
            PerformanceCounterId id = static_cast<PerformanceCounterId>(i + 1);
            counterSet->AddCounter(id, types[i]);
        }

        auto registeredCounterTypes = counterSet->CounterTypes;

        for (size_t i = 0; i < types.size(); ++i)
        {
            PerformanceCounterId id = static_cast<PerformanceCounterId>(i + 1);
            
            auto it = registeredCounterTypes.find(id);

            VERIFY_ARE_NOT_EQUAL(end(registeredCounterTypes), it);
            VERIFY_ARE_EQUAL(types[i], it->second);
        }
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterSetInstance)
    {
        Guid providerId = Guid::NewGuid();
        Guid counterSetId = Guid::NewGuid();

        // create the perf counter instance (implementation will assert if perf lib apis fail)
        auto counterSet = std::make_shared<PerformanceCounterSet>(providerId, counterSetId, PerformanceCounterSetInstanceType::Multiple);

        VERIFY_IS_NOT_NULL(counterSet);

        counterSet->AddCounter(1, PerformanceCounterType::RawData64);
        counterSet->AddCounter(2, PerformanceCounterType::AverageTimer32);
        counterSet->AddCounter(3, PerformanceCounterType::AverageBase);

        std::wstring instanceName = L"counterSetInstanceName";

        auto instance = counterSet->CreateCounterSetInstance(instanceName);

        VERIFY_IS_NOT_NULL(instance);

        // check that the underlying store is accessed by reference
        PerformanceCounterData & counter1 = instance->GetCounter(1);
        PerformanceCounterData & counter2 = instance->GetCounter(2);
        PerformanceCounterData & counter3 = instance->GetCounter(3);

        counter1.Value = 54;
        counter2.Value = 111;

        counter3.Value = ((LONGLONG)(0xFF) << 32) + 1;

        PerformanceCounterData & counterA = instance->GetCounter(1);
        PerformanceCounterData & counterB = instance->GetCounter(2);
        PerformanceCounterData & counterC = instance->GetCounter(3);

        VERIFY_ARE_EQUAL(54, counterA.Value);
        VERIFY_ARE_EQUAL(111, counterB.Value);
        VERIFY_ARE_EQUAL(((LONGLONG)(0xFF) << 32) + 1, counterC.Value);

        VERIFY_ARE_EQUAL(counter1.Value, counterA.Value);
        VERIFY_ARE_EQUAL(counter2.Value, counterB.Value);
        VERIFY_ARE_EQUAL(counter3.Value, counterC.Value);

#if !defined(PLATFORM_UNIX)
        // verify the counter info (checks that counterset created it correctly)
        PERF_COUNTERSET_INSTANCE* instanceInfo = ::PerfQueryInstance(counterSet->ProviderHandle, &counterSetId.AsGUID(), instanceName.c_str(), 0);

        VERIFY_IS_NOT_NULL(instanceInfo);

        VERIFY_ARE_EQUAL(counterSetId, Guid(instanceInfo->CounterSetGuid));
        VERIFY_ARE_EQUAL((ULONG)0, instanceInfo->InstanceId);
        VERIFY_ARE_EQUAL(instanceName, std::wstring((wchar_t*)((BYTE*)(instanceInfo) + instanceInfo->InstanceNameOffset)));
#endif
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterDefinition)
    {
        std::vector<PerformanceCounterType::Enum> types;
        types.push_back(PerformanceCounterType::QueueLength);
        types.push_back(PerformanceCounterType::LargeQueueLength);
        types.push_back(PerformanceCounterType::QueueLength100Ns );
        types.push_back(PerformanceCounterType::QueueLengthObjectTime);
        types.push_back(PerformanceCounterType::RawData32);
        types.push_back(PerformanceCounterType::RawData64);
        types.push_back(PerformanceCounterType::RawDataHex32);
        types.push_back(PerformanceCounterType::RawDataHex64);
        types.push_back(PerformanceCounterType::RateOfCountPerSecond32);
        types.push_back(PerformanceCounterType::RateOfCountPerSecond64);
        types.push_back(PerformanceCounterType::RawFraction32);
        types.push_back(PerformanceCounterType::RawFraction64);
        types.push_back(PerformanceCounterType::RawBase32);
        types.push_back(PerformanceCounterType::RawBase64);
        types.push_back(PerformanceCounterType::SampleFraction);
        types.push_back(PerformanceCounterType::SampleCounter);
        types.push_back(PerformanceCounterType::SampleBase);
        types.push_back(PerformanceCounterType::AverageTimer32);
        types.push_back(PerformanceCounterType::AverageBase);
        types.push_back(PerformanceCounterType::AverageCount64);
        types.push_back(PerformanceCounterType::PercentageActive);
        types.push_back(PerformanceCounterType::PercentageNotActive);
        types.push_back(PerformanceCounterType::PercentageActive100Ns);
        types.push_back(PerformanceCounterType::PercentageNotActive100Ns);
        types.push_back(PerformanceCounterType::ElapsedTime);
        types.push_back(PerformanceCounterType::MultiTimerPercentageActive);
        types.push_back(PerformanceCounterType::MultiTimerPercentageNotActive);
        types.push_back(PerformanceCounterType::MultiTimerPercentageActive100Ns);
        types.push_back(PerformanceCounterType::MultiTimerPercentageNotActive100Ns);
        types.push_back(PerformanceCounterType::MultiTimerBase);
        types.push_back(PerformanceCounterType::Delta32);
        types.push_back(PerformanceCounterType::Delta64);
        types.push_back(PerformanceCounterType::ObjectSpecificTimer);
        types.push_back(PerformanceCounterType::PrecisionSystemTimer);
        types.push_back(PerformanceCounterType::PrecisionTimer100Ns);
        types.push_back(PerformanceCounterType::PrecisionObjectSpecificTimer);

        PerformanceCounterId id = 0;

        std::wstring name(L"CounterName");
        std::wstring description(L"CounterDescription");

        for (auto it = begin(types); it != end(types); ++it)
        {
            PerformanceCounterDefinition definition(id, *it, name, description);

            VERIFY_ARE_EQUAL(id, definition.Identifier);
            VERIFY_ARE_EQUAL(*it, definition.Type);
            VERIFY_ARE_EQUAL(name, definition.Name);
            VERIFY_ARE_EQUAL(description, definition.Description);

            PerformanceCounterDefinition copy(definition);

            VERIFY_ARE_EQUAL(id, copy.Identifier);
            VERIFY_ARE_EQUAL(*it, copy.Type);
            VERIFY_ARE_EQUAL(name, copy.Name);
            VERIFY_ARE_EQUAL(description, copy.Description);

            PerformanceCounterDefinition moved(std::move(copy));

            VERIFY_ARE_EQUAL(id, moved.Identifier);
            VERIFY_ARE_EQUAL(*it, moved.Type);
            VERIFY_ARE_EQUAL(name, moved.Name);
            VERIFY_ARE_EQUAL(description, moved.Description);
        }
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterSetDefinition)
    {
        std::wstring name(L"counterSetName");
        std::wstring description(L"counterSetDescription");

        std::vector<PerformanceCounterSetInstanceType::Enum> types;
        types.push_back(PerformanceCounterSetInstanceType::Single);
        types.push_back(PerformanceCounterSetInstanceType::Multiple);
        types.push_back(PerformanceCounterSetInstanceType::GlobalAggregate);
        types.push_back(PerformanceCounterSetInstanceType::GlobalAggregateWithHistory);
        types.push_back(PerformanceCounterSetInstanceType::MultipleAggregate);
        types.push_back(PerformanceCounterSetInstanceType::InstanceAggregate);

        for (size_t counterCount = 0; counterCount < 3; ++counterCount)
        {
            for (auto it = begin(types); end(types) != it; ++it)
            {
                Guid counterSetId = Guid::NewGuid();

                PerformanceCounterSetDefinition definition(counterSetId, name, description, *it);

                for (size_t i = 0; i < counterCount; ++i)
                {
                    std::wstring counterName(L"name");
                    std::wstring counterDescription(L"description");

                    PerformanceCounterDefinition counter((PerformanceCounterId)i + 1, PerformanceCounterType::RawBase64, counterName, counterDescription);

                    definition.AddCounterDefinition(std::move(counter));
                }

                VERIFY_ARE_EQUAL(name, definition.Name);
                VERIFY_ARE_EQUAL(description, definition.Description);
                VERIFY_ARE_EQUAL(*it, definition.InstanceType);
                VERIFY_ARE_EQUAL(counterSetId, definition.Identifier);
                VERIFY_ARE_EQUAL(counterCount, definition.CounterDefinitions.size());

                PerformanceCounterSetDefinition copy(definition);

                VERIFY_ARE_EQUAL(name, copy.Name);
                VERIFY_ARE_EQUAL(description, copy.Description);
                VERIFY_ARE_EQUAL(*it, copy.InstanceType);
                VERIFY_ARE_EQUAL(counterSetId, copy.Identifier);
                VERIFY_ARE_EQUAL(counterCount, copy.CounterDefinitions.size());

                PerformanceCounterSetDefinition moved(std::move(copy));

                VERIFY_ARE_EQUAL(name, moved.Name);
                VERIFY_ARE_EQUAL(description, moved.Description);
                VERIFY_ARE_EQUAL(*it, moved.InstanceType);
                VERIFY_ARE_EQUAL(counterSetId, moved.Identifier);
                VERIFY_ARE_EQUAL(counterCount, moved.CounterDefinitions.size());
            }
        }
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceProviderDefinition)
    {
        PerformanceProviderDefinition provider;
        
        Guid counterSetId1 = Guid::NewGuid();

        PerformanceCounterSetDefinition counterSet1(
            counterSetId1, 
            counterSetId1.ToString(), 
            counterSetId1.ToString(), 
            PerformanceCounterSetInstanceType::Multiple);

        provider.AddCounterSetDefinition(counterSet1);

        auto & retrieved = provider.GetCounterSetDefinition(counterSetId1);

        VERIFY_ARE_EQUAL(counterSetId1, retrieved.Identifier);

        Guid counterSetId2 = Guid::NewGuid();

        PerformanceCounterSetDefinition counterSet2(
            counterSetId2, 
            counterSetId2.ToString(), 
            counterSetId2.ToString(), 
            PerformanceCounterSetInstanceType::Multiple);

        provider.AddCounterSetDefinition(counterSet2);

        auto & retrieved2 = provider.GetCounterSetDefinition(counterSetId2);

        VERIFY_ARE_EQUAL(counterSetId2, retrieved2.Identifier);
    }

    BOOST_AUTO_TEST_CASE(TestPerformanceCounterMacros)
    {
        std::wstring instanceName1(L"instance1");

        auto instance1 = TestCounterSet1::CreateInstance(instanceName1);
        
        // check counter creation against underlying PerformanceCounterSetInstance
        instance1->Count.Value = 100;
        instance1->TimerBase.Value = 200;
        instance1->Timer.Value = 300;

        VERIFY_ARE_EQUAL(100, instance1->Count.Value);
        VERIFY_ARE_EQUAL(200, instance1->TimerBase.Value);
        VERIFY_ARE_EQUAL(300, instance1->Timer.Value);

        auto & count = instance1->Instance().GetCounter(1);
        auto & timerBase = instance1->Instance().GetCounter(2);
        auto & timer = instance1->Instance().GetCounter(3);

        VERIFY_ARE_EQUAL(100, count.Value);
        VERIFY_ARE_EQUAL(200, timerBase.Value);
        VERIFY_ARE_EQUAL(300, timer.Value);

        // check metadata
        auto & definition = PerformanceProviderDefinition::Singleton()->GetCounterSetDefinition(testCounterSet1Id);

        VERIFY_ARE_EQUAL(testCounterSet1Id, definition.Identifier);
        VERIFY_ARE_EQUAL(testCounterSet1Id, Common::Guid(definition.Name));
        VERIFY_ARE_EQUAL(testCounterSet1Id, Common::Guid(definition.Description));

        VERIFY_IS_TRUE(end(definition.CounterDefinitions) != definition.CounterDefinitions.find(1));
        VERIFY_IS_TRUE(end(definition.CounterDefinitions) != definition.CounterDefinitions.find(2));
        VERIFY_IS_TRUE(end(definition.CounterDefinitions) != definition.CounterDefinitions.find(3));
        
        auto counterDef1 = definition.CounterDefinitions.find(1)->second;
        auto counterDef2 = definition.CounterDefinitions.find(2)->second;
        auto counterDef3 = definition.CounterDefinitions.find(3)->second;

        VERIFY_ARE_EQUAL(PerformanceCounterType::RawBase64, counterDef1.Type);
        VERIFY_ARE_EQUAL(std::wstring(L"counter1"), counterDef1.Name);
        VERIFY_ARE_EQUAL(std::wstring(L"counter1 description"), counterDef1.Description);

        VERIFY_ARE_EQUAL(PerformanceCounterType::AverageBase, counterDef2.Type);
        VERIFY_ARE_EQUAL(std::wstring(L"counter2"), counterDef2.Name);
        VERIFY_ARE_EQUAL(std::wstring(L""), counterDef2.Description);

        VERIFY_ARE_EQUAL(PerformanceCounterType::AverageTimer32, counterDef3.Type);
        VERIFY_ARE_EQUAL(std::wstring(L"counter3"), counterDef3.Name);
        VERIFY_ARE_EQUAL(std::wstring(L"counter3 description"), counterDef3.Description);

        // check counterset
        auto & counterSet = TestCounterSet1::GetCounterSet();

        for (auto it = begin(counterSet.CounterTypes); end(counterSet.CounterTypes) != it; ++it)
        {
            auto & counterDef = definition.CounterDefinitions.find(it->first)->second;

            VERIFY_ARE_EQUAL(counterDef.Type, it->second);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
