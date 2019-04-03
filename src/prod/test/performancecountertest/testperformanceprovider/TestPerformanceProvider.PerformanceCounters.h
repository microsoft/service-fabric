// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TestPerformanceProvider
{
    // the provider in common is hardcoded to the product provider, we can define our own provider here 
    // and adjust the macros to use it instead
    class TestPerformanceProviderDefinition
    {
        DENY_COPY(TestPerformanceProviderDefinition)

    public:
        TestPerformanceProviderDefinition(void);
        ~TestPerformanceProviderDefinition(void);

        void AddCounterSetDefinition(PerformanceCounterSetDefinition & performanceCounterSetDefinition);
        void AddCounterSetDefinition(PerformanceCounterSetDefinition && performanceCounterSetDefinition);

        PerformanceCounterSetDefinition const & GetCounterSetDefinition(Guid const & counterSetId);

        inline static TestPerformanceProviderDefinition* Singleton()
        {
            return singletonInstance_;
        }

        __declspec(property(get=get_Identifier)) Guid const & Identifier;
        inline Guid const & get_Identifier() const { return identifier_; }

    private:

        static Guid identifier_;
        
        static TestPerformanceProviderDefinition* singletonInstance_;

        std::map<Guid, PerformanceCounterSetDefinition> counterSetDefinitions_;

        RwLock counterSetDefinitionsLock_;
    };

    // varition of the macros in PerformanceCounter.h to add the counterset definitions to the test provider

#ifdef END_COUNTER_SET_DEFINITION
#undef END_COUNTER_SET_DEFINITION
#endif

    #define END_COUNTER_SET_DEFINITION() \
        TestPerformanceProviderDefinition::Singleton()->AddCounterSetDefinition(std::move(counterSet)); \
        return TestPerformanceProviderDefinition::Singleton()->GetCounterSetDefinition(counterSetId); \
    } \

    #define INITIALIZE_TEST_COUNTER_SET(className) \
    namespace \
    { \
        static Common::PerformanceCounterSetSPtr className##__CounterSet; \
        static INIT_ONCE initOnce_##className; \
        static BOOL CALLBACK InitOnce##className(PINIT_ONCE, PVOID, PVOID*) \
        { \
            Common::PerformanceCounterSetDefinition const & definition = className::Initialize(); \
            className##__CounterSet = std::make_shared<Common::PerformanceCounterSet>( \
                TestPerformanceProviderDefinition::Singleton()->Identifier, \
                definition.Identifier, \
                definition.InstanceType); \
            auto counters = definition.CounterDefinitions; \
            for (auto it = begin(counters); end(counters) != it; ++it) \
            { \
                className##__CounterSet->AddCounter(it->second.Identifier, it->second.Type); \
            } \
            return TRUE; \
        } \
    } \
    Common::PerformanceCounterSet & className::GetCounterSet(void) \
    { \
        BOOL  bStatus = FALSE; \
        PVOID lpContext = NULL; \
        bStatus = ::InitOnceExecuteOnce( \
            &initOnce_##className, \
            InitOnce##className, \
            NULL, \
            &lpContext); \
        return *className##__CounterSet; \
    } \

    class TestCounterSet1
    {
        DENY_COPY(TestCounterSet1)

        BEGIN_COUNTER_SET_DEFINITION(
            L"9625B850-5F96-487B-9B3D-C0FC0359EF20", 
            L"CIT TestCounterSet1",
            L"This is a test counter set. If you see this on a production machine, you are holding it wrong.",
            Common::PerformanceCounterSetInstanceType::Multiple)

            COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"Counter 1", L"Description of Counter 1")
            COUNTER_DEFINITION(2, Common::PerformanceCounterType::RawData64, L"Counter 2", L"Description of Counter 2")
            COUNTER_DEFINITION(3, Common::PerformanceCounterType::RawData64, L"Counter 3", L"Description of Counter 3")
            COUNTER_DEFINITION(4, Common::PerformanceCounterType::RawData64, L"Counter 4", L"Description of Counter 4")

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(Counter1)
        DECLARE_COUNTER_INSTANCE(Counter2)
        DECLARE_COUNTER_INSTANCE(Counter3)
        DECLARE_COUNTER_INSTANCE(Counter4)

        BEGIN_COUNTER_SET_INSTANCE(TestCounterSet1)
            DEFINE_COUNTER_INSTANCE(Counter1, 1)
            DEFINE_COUNTER_INSTANCE(Counter2, 2)
            DEFINE_COUNTER_INSTANCE(Counter3, 3)
            DEFINE_COUNTER_INSTANCE(Counter4, 4)
        END_COUNTER_SET_INSTANCE()
    };

    class TestCounterSet2
    {
        DENY_COPY(TestCounterSet2)

        BEGIN_COUNTER_SET_DEFINITION(
            L"F301D1D0-9CB6-4297-85E5-07BCE6A3A630", 
            L"CIT TestCounterSet2",
            L"This is another test counter set. If you see this on a production machine, you are holding it wrong.",
            Common::PerformanceCounterSetInstanceType::Multiple)

            COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"Counter 1", L"Description of Counter 1")
            COUNTER_DEFINITION(2, Common::PerformanceCounterType::RawData64, L"Counter 2", L"Description of Counter 2")
            COUNTER_DEFINITION(3, Common::PerformanceCounterType::RawData64, L"Counter 3", L"Description of Counter 3")
            COUNTER_DEFINITION(4, Common::PerformanceCounterType::RawData64, L"Counter 4", L"Description of Counter 4")

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(Counter1)
        DECLARE_COUNTER_INSTANCE(Counter2)
        DECLARE_COUNTER_INSTANCE(Counter3)
        DECLARE_COUNTER_INSTANCE(Counter4)

        BEGIN_COUNTER_SET_INSTANCE(TestCounterSet2)
            DEFINE_COUNTER_INSTANCE(Counter1, 1)
            DEFINE_COUNTER_INSTANCE(Counter2, 2)
            DEFINE_COUNTER_INSTANCE(Counter3, 3)
            DEFINE_COUNTER_INSTANCE(Counter4, 4)
        END_COUNTER_SET_INSTANCE()
    };
}
