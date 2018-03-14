// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    typedef std::map<PerformanceCounterId, PerformanceCounterType::Enum> CounterIdToType;

    //
    // Represents a counter set. A counter set is a collection of counters that form a unit of instantiation.
    //
    class PerformanceCounterSet : public std::enable_shared_from_this<PerformanceCounterSet>
    {
        DENY_COPY(PerformanceCounterSet)

    public:
        PerformanceCounterSet(Guid const & providerId, Guid const & counterSetId, PerformanceCounterSetInstanceType::Enum instanceType);
        ~PerformanceCounterSet(void);

        void AddCounter(PerformanceCounterId counterId, PerformanceCounterType::Enum PerformanceCounterType);

        PerformanceCounterSetInstanceSPtr CreateCounterSetInstance(std::wstring const & instanceName);
        HRESULT CreateCounterSetInstance(std::wstring const & instanceName, bool allocateCounterMemory, bool avoidAssert, PerformanceCounterSetInstance* &counterSetInstance);

        __declspec(property(get=get_CounterSetId)) Guid const & CounterSetId;
        inline Guid const & get_CounterSetId() const { return counterSetId_; }

        __declspec(property(get=get_ProviderHandle)) HANDLE ProviderHandle;
        inline HANDLE get_ProviderHandle() { return provider_->Handle(); }

        __declspec(property(get=get_CounterTypes)) CounterIdToType const & CounterTypes;
        inline CounterIdToType const & get_CounterTypes() const { return counterTypes_; }

    private:

        long nextCounterInstanceId_;

        // initialize the counter set if necessary
        HRESULT EnsureInitialized(bool);

        // the instance type used to create the counter set info
        PerformanceCounterSetInstanceType::Enum instanceType_;

        // the id of the provider that contains the counter set
        Guid providerId_;

        // the id of the counter set
        Guid counterSetId_;

        // specifies the counter type for every counter (CounterId -> PerformanceCounterType)
        CounterIdToType counterTypes_;

        // indicates whether the counter set has been initialized
        // the counter set must be initialized before creating an instance
        bool isInitialized_;

        // protects the initialization of the counter set
        RwLock initializationLock_;

        // the provider that contains the counter set
        PerformanceProviderSPtr provider_;
    
    };

    typedef std::shared_ptr<PerformanceCounterSet> PerformanceCounterSetSPtr;
}

