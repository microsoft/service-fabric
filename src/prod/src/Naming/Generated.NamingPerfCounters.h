
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once























namespace Naming
{
    class NamingPerformanceCounters
    {
        DENY_COPY(NamingPerformanceCounters)

    public:
    
        BEGIN_COUNTER_SET_DEFINITION(
            L"7CF963F9-5E87-42F0-BAEE-B98C86206BA6",
            L"Naming Service",
            L"Counters for Naming Service",
            Common::PerformanceCounterSetInstanceType::Multiple)

            COUNTER_DEFINITION( 0+1, Common::PerformanceCounterType::RateOfCountPerSecond32, L"AO Create Name req/sec", L"Incoming (authority owner) create name requests per second" )
            COUNTER_DEFINITION( 0+2, Common::PerformanceCounterType::RateOfCountPerSecond32, L"AO Delete Name req/sec", L"Incoming (authority owner) delete name requests per second" )
            COUNTER_DEFINITION( 0+3, Common::PerformanceCounterType::RateOfCountPerSecond32, L"NO Create Name req/sec", L"Incoming (name owner) create name requests per second" )
            COUNTER_DEFINITION( 0+4, Common::PerformanceCounterType::RateOfCountPerSecond32, L"NO Delete Name req/sec", L"Incoming (name owner) delete name requests per second" )
            COUNTER_DEFINITION( 0+5, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Name Exists req/sec", L"Incoming name exists requests per second" )
            COUNTER_DEFINITION( 0+6, Common::PerformanceCounterType::RateOfCountPerSecond32, L"AO Create Service req/sec", L"Incoming (authority owner) create service requests per second" )
            COUNTER_DEFINITION( 0+7, Common::PerformanceCounterType::RateOfCountPerSecond32, L"AO Update Service req/sec", L"Incoming (authority owner) update service requests per second" )
            COUNTER_DEFINITION( 0+8, Common::PerformanceCounterType::RateOfCountPerSecond32, L"AO Delete Service req/sec", L"Incoming (authority owner) delete service requests per second" )
            COUNTER_DEFINITION( 0+9, Common::PerformanceCounterType::RateOfCountPerSecond32, L"NO Create Service req/sec", L"Incoming (name owner) create service requests per second" )
            COUNTER_DEFINITION( 0+10, Common::PerformanceCounterType::RateOfCountPerSecond32, L"NO Update Service req/sec", L"Incoming (name owner) update service requests per second" )
            COUNTER_DEFINITION( 0+11, Common::PerformanceCounterType::RateOfCountPerSecond32, L"NO Delete Service req/sec", L"Incoming (name owner) delete service requests per second" )
            COUNTER_DEFINITION( 0+12, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Enumerate Names req/sec", L"Incoming enumerate names requests per second" )
            COUNTER_DEFINITION( 0+13, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Enumerate Properties req/sec", L"Incoming enumerate properties requests per second" )
            COUNTER_DEFINITION( 0+14, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Property Batch req/sec", L"Incoming property batch per second" )
            COUNTER_DEFINITION( 0+15, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Get Service Description req/sec", L"Incoming get service description requests per second" )
            COUNTER_DEFINITION( 0+16, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Prefix Resolve req/sec", L"Incoming prefix resolve requests per second" )
            COUNTER_DEFINITION( 0+17, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Unrecognized Operation req/sec", L"Incoming unrecognized operation per second" )

            COUNTER_DEFINITION( 50+1, Common::PerformanceCounterType::AverageBase, L"Base for AO Create Name avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+2, Common::PerformanceCounterType::AverageBase, L"Base for AO Delete Name avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+3, Common::PerformanceCounterType::AverageBase, L"Base for NO Create Name avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+4, Common::PerformanceCounterType::AverageBase, L"Base for NO Delete Name avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+5, Common::PerformanceCounterType::AverageBase, L"Base for Name Exists avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+6, Common::PerformanceCounterType::AverageBase, L"Base for AO Create Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+7, Common::PerformanceCounterType::AverageBase, L"Base for AO Update Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+8, Common::PerformanceCounterType::AverageBase, L"Base for AO Delete Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+9, Common::PerformanceCounterType::AverageBase, L"Base for NO Create Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+10, Common::PerformanceCounterType::AverageBase, L"Base for NO Update Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+11, Common::PerformanceCounterType::AverageBase, L"Base for NO Delete Service avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+12, Common::PerformanceCounterType::AverageBase, L"Base for Enumerate Names avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+13, Common::PerformanceCounterType::AverageBase, L"Base for Enumerate Properties avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+14, Common::PerformanceCounterType::AverageBase, L"Base for Property Batch avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+15, Common::PerformanceCounterType::AverageBase, L"Base for Get Service Description avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+16, Common::PerformanceCounterType::AverageBase, L"Base for Prefix Resolve avg. duration (us)", L"", noDisplay)
            COUNTER_DEFINITION( 50+17, Common::PerformanceCounterType::AverageBase, L"Base for Unrecognized Operation avg. duration (us)", L"", noDisplay)

            COUNTER_DEFINITION_WITH_BASE( 100+1, 50+1, Common::PerformanceCounterType::AverageCount64, L"AO Create Name avg. duration (us)", L"Average (authority owner) create name processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+2, 50+2, Common::PerformanceCounterType::AverageCount64, L"AO Delete Name avg. duration (us)", L"Average (authority owner) delete name processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+3, 50+3, Common::PerformanceCounterType::AverageCount64, L"NO Create Name avg. duration (us)", L"Average (name owner) create name processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+4, 50+4, Common::PerformanceCounterType::AverageCount64, L"NO Delete Name avg. duration (us)", L"Average (name owner) delete name processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+5, 50+5, Common::PerformanceCounterType::AverageCount64, L"Name Exists avg. duration (us)", L"Average name exists processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+6, 50+6, Common::PerformanceCounterType::AverageCount64, L"AO Create Service avg. duration (us)", L"Average (authority owner) create service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+7, 50+7, Common::PerformanceCounterType::AverageCount64, L"AO Update Service avg. duration (us)", L"Average (authority owner) update service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+8, 50+8, Common::PerformanceCounterType::AverageCount64, L"AO Delete Service avg. duration (us)", L"Average (authority owner) delete service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+9, 50+9, Common::PerformanceCounterType::AverageCount64, L"NO Create Service avg. duration (us)", L"Average (name owner) create service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+10, 50+10, Common::PerformanceCounterType::AverageCount64, L"NO Update Service avg. duration (us)", L"Average (name owner) update service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+11, 50+11, Common::PerformanceCounterType::AverageCount64, L"NO Delete Service avg. duration (us)", L"Average (name owner) delete service processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+12, 50+12, Common::PerformanceCounterType::AverageCount64, L"Enumerate Names avg. duration (us)", L"Average enumerate names processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+13, 50+13, Common::PerformanceCounterType::AverageCount64, L"Enumerate Properties avg. duration (us)", L"Average enumerate properties processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+14, 50+14, Common::PerformanceCounterType::AverageCount64, L"Property Batch avg. duration (us)", L"Incoming property batch per second" )
            COUNTER_DEFINITION_WITH_BASE( 100+15, 50+15, Common::PerformanceCounterType::AverageCount64, L"Get Service Description avg. duration (us)", L"Average get service description processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+16, 50+16, Common::PerformanceCounterType::AverageCount64, L"Prefix Resolve avg. duration (us)", L"Average prefix resolution processing time in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 100+17, 50+17, Common::PerformanceCounterType::AverageCount64, L"Unrecognized Operation avg. duration (us)", L"Incoming unrecognized operation processing time in microseconds" )

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE( RateOfAOCreateName )
        DECLARE_COUNTER_INSTANCE( RateOfAODeleteName )
        DECLARE_COUNTER_INSTANCE( RateOfNOCreateName )
        DECLARE_COUNTER_INSTANCE( RateOfNODeleteName )
        DECLARE_COUNTER_INSTANCE( RateOfNameExists )
        DECLARE_COUNTER_INSTANCE( RateOfAOCreateService )
        DECLARE_COUNTER_INSTANCE( RateOfAOUpdateService )
        DECLARE_COUNTER_INSTANCE( RateOfAODeleteService )
        DECLARE_COUNTER_INSTANCE( RateOfNOCreateService )
        DECLARE_COUNTER_INSTANCE( RateOfNOUpdateService )
        DECLARE_COUNTER_INSTANCE( RateOfNODeleteService )
        DECLARE_COUNTER_INSTANCE( RateOfEnumerateNames )
        DECLARE_COUNTER_INSTANCE( RateOfEnumerateProperties )
        DECLARE_COUNTER_INSTANCE( RateOfPropertyBatch )
        DECLARE_COUNTER_INSTANCE( RateOfGetServiceDescription )
        DECLARE_COUNTER_INSTANCE( RateOfPrefixResolve )
        DECLARE_COUNTER_INSTANCE( RateOfInvalidOperation )

        DECLARE_COUNTER_INSTANCE( DurationOfAOCreateNameBase )
        DECLARE_COUNTER_INSTANCE( DurationOfAODeleteNameBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNOCreateNameBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNODeleteNameBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNameExistsBase )
        DECLARE_COUNTER_INSTANCE( DurationOfAOCreateServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfAOUpdateServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfAODeleteServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNOCreateServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNOUpdateServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfNODeleteServiceBase )
        DECLARE_COUNTER_INSTANCE( DurationOfEnumerateNamesBase )
        DECLARE_COUNTER_INSTANCE( DurationOfEnumeratePropertiesBase )
        DECLARE_COUNTER_INSTANCE( DurationOfPropertyBatchBase )
        DECLARE_COUNTER_INSTANCE( DurationOfGetServiceDescriptionBase )
        DECLARE_COUNTER_INSTANCE( DurationOfPrefixResolveBase )
        DECLARE_COUNTER_INSTANCE( DurationOfInvalidOperationBase )

        DECLARE_COUNTER_INSTANCE( DurationOfAOCreateName )
        DECLARE_COUNTER_INSTANCE( DurationOfAODeleteName )
        DECLARE_COUNTER_INSTANCE( DurationOfNOCreateName )
        DECLARE_COUNTER_INSTANCE( DurationOfNODeleteName )
        DECLARE_COUNTER_INSTANCE( DurationOfNameExists )
        DECLARE_COUNTER_INSTANCE( DurationOfAOCreateService )
        DECLARE_COUNTER_INSTANCE( DurationOfAOUpdateService )
        DECLARE_COUNTER_INSTANCE( DurationOfAODeleteService )
        DECLARE_COUNTER_INSTANCE( DurationOfNOCreateService )
        DECLARE_COUNTER_INSTANCE( DurationOfNOUpdateService )
        DECLARE_COUNTER_INSTANCE( DurationOfNODeleteService )
        DECLARE_COUNTER_INSTANCE( DurationOfEnumerateNames )
        DECLARE_COUNTER_INSTANCE( DurationOfEnumerateProperties )
        DECLARE_COUNTER_INSTANCE( DurationOfPropertyBatch )
        DECLARE_COUNTER_INSTANCE( DurationOfGetServiceDescription )
        DECLARE_COUNTER_INSTANCE( DurationOfPrefixResolve )
        DECLARE_COUNTER_INSTANCE( DurationOfInvalidOperation )

        BEGIN_COUNTER_SET_INSTANCE(NamingPerformanceCounters)
            DEFINE_COUNTER_INSTANCE( RateOfAOCreateName, 0+1 )
            DEFINE_COUNTER_INSTANCE( RateOfAODeleteName, 0+2 )
            DEFINE_COUNTER_INSTANCE( RateOfNOCreateName, 0+3 )
            DEFINE_COUNTER_INSTANCE( RateOfNODeleteName, 0+4 )
            DEFINE_COUNTER_INSTANCE( RateOfNameExists, 0+5 )
            DEFINE_COUNTER_INSTANCE( RateOfAOCreateService, 0+6 )
            DEFINE_COUNTER_INSTANCE( RateOfAOUpdateService, 0+7 )
            DEFINE_COUNTER_INSTANCE( RateOfAODeleteService, 0+8 )
            DEFINE_COUNTER_INSTANCE( RateOfNOCreateService, 0+9 )
            DEFINE_COUNTER_INSTANCE( RateOfNOUpdateService, 0+10 )
            DEFINE_COUNTER_INSTANCE( RateOfNODeleteService, 0+11 )
            DEFINE_COUNTER_INSTANCE( RateOfEnumerateNames, 0+12 )
            DEFINE_COUNTER_INSTANCE( RateOfEnumerateProperties, 0+13 )
            DEFINE_COUNTER_INSTANCE( RateOfPropertyBatch, 0+14 )
            DEFINE_COUNTER_INSTANCE( RateOfGetServiceDescription, 0+15 )
            DEFINE_COUNTER_INSTANCE( RateOfPrefixResolve, 0+16 )
            DEFINE_COUNTER_INSTANCE( RateOfInvalidOperation, 0+17 )

            DEFINE_COUNTER_INSTANCE( DurationOfAOCreateNameBase, 50+1 )
            DEFINE_COUNTER_INSTANCE( DurationOfAODeleteNameBase, 50+2 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOCreateNameBase, 50+3 )
            DEFINE_COUNTER_INSTANCE( DurationOfNODeleteNameBase, 50+4 )
            DEFINE_COUNTER_INSTANCE( DurationOfNameExistsBase, 50+5 )
            DEFINE_COUNTER_INSTANCE( DurationOfAOCreateServiceBase, 50+6 )
            DEFINE_COUNTER_INSTANCE( DurationOfAOUpdateServiceBase, 50+7 )
            DEFINE_COUNTER_INSTANCE( DurationOfAODeleteServiceBase, 50+8 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOCreateServiceBase, 50+9 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOUpdateServiceBase, 50+10 )
            DEFINE_COUNTER_INSTANCE( DurationOfNODeleteServiceBase, 50+11 )
            DEFINE_COUNTER_INSTANCE( DurationOfEnumerateNamesBase, 50+12 )
            DEFINE_COUNTER_INSTANCE( DurationOfEnumeratePropertiesBase, 50+13 )
            DEFINE_COUNTER_INSTANCE( DurationOfPropertyBatchBase, 50+14 )
            DEFINE_COUNTER_INSTANCE( DurationOfGetServiceDescriptionBase, 50+15 )
            DEFINE_COUNTER_INSTANCE( DurationOfPrefixResolveBase, 50+16 )
            DEFINE_COUNTER_INSTANCE( DurationOfInvalidOperationBase, 50+17 )

            DEFINE_COUNTER_INSTANCE( DurationOfAOCreateName, 100+1 )
            DEFINE_COUNTER_INSTANCE( DurationOfAODeleteName, 100+2 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOCreateName, 100+3 )
            DEFINE_COUNTER_INSTANCE( DurationOfNODeleteName, 100+4 )
            DEFINE_COUNTER_INSTANCE( DurationOfNameExists, 100+5 )
            DEFINE_COUNTER_INSTANCE( DurationOfAOCreateService, 100+6 )
            DEFINE_COUNTER_INSTANCE( DurationOfAOUpdateService, 100+7 )
            DEFINE_COUNTER_INSTANCE( DurationOfAODeleteService, 100+8 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOCreateService, 100+9 )
            DEFINE_COUNTER_INSTANCE( DurationOfNOUpdateService, 100+10 )
            DEFINE_COUNTER_INSTANCE( DurationOfNODeleteService, 100+11 )
            DEFINE_COUNTER_INSTANCE( DurationOfEnumerateNames, 100+12 )
            DEFINE_COUNTER_INSTANCE( DurationOfEnumerateProperties, 100+13 )
            DEFINE_COUNTER_INSTANCE( DurationOfPropertyBatch, 100+14 )
            DEFINE_COUNTER_INSTANCE( DurationOfGetServiceDescription, 100+15 )
            DEFINE_COUNTER_INSTANCE( DurationOfPrefixResolve, 100+16 )
            DEFINE_COUNTER_INSTANCE( DurationOfInvalidOperation, 100+17 )
        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<NamingPerformanceCounters> PerformanceCountersSPtr;
}
