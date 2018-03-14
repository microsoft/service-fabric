// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class PerformanceProviderDefinition
    {
        DENY_COPY(PerformanceProviderDefinition)

    public:
        PerformanceProviderDefinition(void);
        ~PerformanceProviderDefinition(void);

        void AddCounterSetDefinition(PerformanceCounterSetDefinition & performanceCounterSetDefinition);
        void AddCounterSetDefinition(PerformanceCounterSetDefinition && performanceCounterSetDefinition);

        PerformanceCounterSetDefinition const & GetCounterSetDefinition(Guid const & counterSetId);

        inline static PerformanceProviderDefinition* Singleton()
        {
            return singletonInstance_;
        }

        __declspec(property(get=get_Identifier)) Guid const & Identifier;
        inline Guid const & get_Identifier() const { return identifier_; }

    private:

        static Guid identifier_;
        
        static PerformanceProviderDefinition* singletonInstance_;

        std::map<Guid, PerformanceCounterSetDefinition> counterSetDefinitions_;

        RwLock counterSetDefinitionsLock_;
    };
}

