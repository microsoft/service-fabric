// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once






























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

                    COUNTER_DEFINITION( 0+1, Common:: PerformanceCounterType::RawData32, L"# of Application Creations", L"Number of applications being created" ) 
                    COUNTER_DEFINITION( 0+2, Common:: PerformanceCounterType::RawData32, L"# of Application Removals", L"Number of applications being removed" )
                    COUNTER_DEFINITION( 0+3, Common:: PerformanceCounterType::RawData32, L"# of Application Upgrades", L"Number of applications being upgraded" )

                END_COUNTER_SET_DEFINITION()

                DECLARE_COUNTER_INSTANCE( NumberOfAppCreates )
                DECLARE_COUNTER_INSTANCE( NumberOfAppRemoves )
                DECLARE_COUNTER_INSTANCE( NumberOfAppUpgrades )


                BEGIN_COUNTER_SET_INSTANCE(ClusterManagerPerformanceCounters)
                    DEFINE_COUNTER_INSTANCE( NumberOfAppCreates, 0+1 )
                    DEFINE_COUNTER_INSTANCE( NumberOfAppRemoves, 0+2 )
                    DEFINE_COUNTER_INSTANCE( NumberOfAppUpgrades, 0+3 )

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

                    COUNTER_DEFINITION( 50+1, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Image Builder Failures/sec", L"Image Builder failures per second" )

                END_COUNTER_SET_DEFINITION()

                DECLARE_COUNTER_INSTANCE( RateOfImageBuilderFailures )
            
                BEGIN_COUNTER_SET_INSTANCE(ImageBuilderPerformanceCounters)
                    DEFINE_COUNTER_INSTANCE( RateOfImageBuilderFailures, 50+1 )
                END_COUNTER_SET_INSTANCE()
        };

        typedef std::shared_ptr<ClusterManagerPerformanceCounters> ClusterManagerPerformanceCountersSPtr;
        typedef std::shared_ptr<ImageBuilderPerformanceCounters> ImageBuilderPerformanceCountersSPtr;
    }
}
