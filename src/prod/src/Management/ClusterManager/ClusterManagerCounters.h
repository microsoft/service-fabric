// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CM_RAW_INDEX 0
#define CM_RATE_INDEX 50
#define CM_AVG_BASE_INDEX 100
#define CM_AVG_INDEX 150

#define CM_RAW_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( CM_RAW_INDEX+Id, Common:: PerformanceCounterType::RawData32, Name, Description )

#define CM_RATE_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( CM_RATE_INDEX+Id, Common::PerformanceCounterType::RateOfCountPerSecond32, Name, Description )

#define CM_AVG_BASE( Id, Name) \
    COUNTER_DEFINITION( CM_AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageBase, Name, L"", noDisplay)

#define CM_AVG_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION_WITH_BASE( CM_AVG_INDEX+Id, CM_AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageCount64, Name, Description )

#define CM_DEFINE_RAW_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, CM_RAW_INDEX+Id )

#define CM_DEFINE_RATE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, CM_RATE_INDEX+Id )

#define CM_DEFINE_AVG_BASE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, CM_AVG_BASE_INDEX+Id )

#define CM_DEFINE_AVG_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, CM_AVG_INDEX+Id )

namespace Management
{
    namespace ClusterManager
    {
       // Generated.PerformanceCounters.h is scraped by a perf counter manifest generation script and 
       // is more verbose to facilitate that generation. Generated.PerformanceCounters.h is generated
       // on build from this file, which is more concise and less error prone when actually defining 
       // the perfcounters.
       //
       class ClusterManagerPerformanceCounters
        {
            DENY_COPY(ClusterManagerPerformanceCounters)

            public:

                BEGIN_COUNTER_SET_DEFINITION(
                    L"95A740BC-9D13-4E56-B0F7-4C59E845BFF3",
                    L"Cluster Manager",
                    L"Counters for Cluster Manager",
                    Common::PerformanceCounterSetInstanceType::Multiple)

                    CM_RAW_COUNTER( 1, L"# of Application Creations", L"Number of applications being created" ) 
                    CM_RAW_COUNTER( 2, L"# of Application Removals", L"Number of applications being removed" )
                    CM_RAW_COUNTER( 3, L"# of Application Upgrades", L"Number of applications being upgraded" )

                END_COUNTER_SET_DEFINITION()

                DECLARE_COUNTER_INSTANCE( NumberOfAppCreates )
                DECLARE_COUNTER_INSTANCE( NumberOfAppRemoves )
                DECLARE_COUNTER_INSTANCE( NumberOfAppUpgrades )


                BEGIN_COUNTER_SET_INSTANCE(ClusterManagerPerformanceCounters)
                    CM_DEFINE_RAW_COUNTER( 1, NumberOfAppCreates )
                    CM_DEFINE_RAW_COUNTER( 2, NumberOfAppRemoves )
                    CM_DEFINE_RAW_COUNTER( 3, NumberOfAppUpgrades )

                END_COUNTER_SET_INSTANCE()
        };

        class ImageBuilderPerformanceCounters
        {
            DENY_COPY(ImageBuilderPerformanceCounters)

            public:
            
                BEGIN_COUNTER_SET_DEFINITION(
                    L"A54DFB51-FCBF-4830-9123-DF9AC6330C03",
                    L"Image Builder",
                    L"Counters for Image Builder",
                    Common::PerformanceCounterSetInstanceType::Multiple)

                    CM_RATE_COUNTER( 1, L"Image Builder Failures/sec", L"Image Builder failures per second" )

                END_COUNTER_SET_DEFINITION()

                DECLARE_COUNTER_INSTANCE( RateOfImageBuilderFailures )
            
                BEGIN_COUNTER_SET_INSTANCE(ImageBuilderPerformanceCounters)
                    CM_DEFINE_RATE_COUNTER( 1, RateOfImageBuilderFailures )
                END_COUNTER_SET_INSTANCE()
        };

        typedef std::shared_ptr<ClusterManagerPerformanceCounters> ClusterManagerPerformanceCountersSPtr;
        typedef std::shared_ptr<ImageBuilderPerformanceCounters> ImageBuilderPerformanceCountersSPtr;
    }
}
