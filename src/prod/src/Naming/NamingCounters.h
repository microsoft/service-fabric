// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RATE_INDEX 0
#define AVG_BASE_INDEX 50
#define DURATION_INDEX 100

#define RATE_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( RATE_INDEX+Id, Common::PerformanceCounterType::RateOfCountPerSecond32, Name, Description )

#define AVG_BASE( Id, Name ) \
    COUNTER_DEFINITION( AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageBase, Name, L"", noDisplay)

#define DURATION_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION_WITH_BASE( DURATION_INDEX+Id, AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageCount64, Name, Description )

#define DEFINE_RATE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, RATE_INDEX+Id )

#define DEFINE_AVG_BASE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, AVG_BASE_INDEX+Id )

#define DEFINE_DURATION_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, DURATION_INDEX+Id )

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

            RATE_COUNTER( 1, L"AO Create Name req/sec", L"Incoming (authority owner) create name requests per second")
            RATE_COUNTER( 2, L"AO Delete Name req/sec", L"Incoming (authority owner) delete name requests per second")
            RATE_COUNTER( 3, L"NO Create Name req/sec", L"Incoming (name owner) create name requests per second")
            RATE_COUNTER( 4, L"NO Delete Name req/sec", L"Incoming (name owner) delete name requests per second")
            RATE_COUNTER( 5, L"Name Exists req/sec", L"Incoming name exists requests per second")
            RATE_COUNTER( 6, L"AO Create Service req/sec", L"Incoming (authority owner) create service requests per second")
            RATE_COUNTER( 7, L"AO Update Service req/sec", L"Incoming (authority owner) update service requests per second")
            RATE_COUNTER( 8, L"AO Delete Service req/sec", L"Incoming (authority owner) delete service requests per second")
            RATE_COUNTER( 9, L"NO Create Service req/sec", L"Incoming (name owner) create service requests per second")
            RATE_COUNTER( 10, L"NO Update Service req/sec", L"Incoming (name owner) update service requests per second")
            RATE_COUNTER( 11, L"NO Delete Service req/sec", L"Incoming (name owner) delete service requests per second")
            RATE_COUNTER( 12, L"Enumerate Names req/sec", L"Incoming enumerate names requests per second")
            RATE_COUNTER( 13, L"Enumerate Properties req/sec", L"Incoming enumerate properties requests per second")
            RATE_COUNTER( 14, L"Property Batch req/sec", L"Incoming property batch per second")
            RATE_COUNTER( 15, L"Get Service Description req/sec", L"Incoming get service description requests per second")
            RATE_COUNTER( 16, L"Prefix Resolve req/sec", L"Incoming prefix resolve requests per second")
            RATE_COUNTER( 17, L"Unrecognized Operation req/sec", L"Incoming unrecognized operation per second")

            AVG_BASE( 1, L"Base for AO Create Name avg. duration (us)" )
            AVG_BASE( 2, L"Base for AO Delete Name avg. duration (us)" )
            AVG_BASE( 3, L"Base for NO Create Name avg. duration (us)" )
            AVG_BASE( 4, L"Base for NO Delete Name avg. duration (us)" )
            AVG_BASE( 5, L"Base for Name Exists avg. duration (us)" )
            AVG_BASE( 6, L"Base for AO Create Service avg. duration (us)" )
            AVG_BASE( 7, L"Base for AO Update Service avg. duration (us)" )
            AVG_BASE( 8, L"Base for AO Delete Service avg. duration (us)" )
            AVG_BASE( 9, L"Base for NO Create Service avg. duration (us)" )
            AVG_BASE( 10, L"Base for NO Update Service avg. duration (us)" )
            AVG_BASE( 11, L"Base for NO Delete Service avg. duration (us)" )
            AVG_BASE( 12, L"Base for Enumerate Names avg. duration (us)" )
            AVG_BASE( 13, L"Base for Enumerate Properties avg. duration (us)" )
            AVG_BASE( 14, L"Base for Property Batch avg. duration (us)" )
            AVG_BASE( 15, L"Base for Get Service Description avg. duration (us)" )
            AVG_BASE( 16, L"Base for Prefix Resolve avg. duration (us)" )
            AVG_BASE( 17, L"Base for Unrecognized Operation avg. duration (us)" )

            DURATION_COUNTER( 1, L"AO Create Name avg. duration (us)", L"Average (authority owner) create name processing time in microseconds")
            DURATION_COUNTER( 2, L"AO Delete Name avg. duration (us)", L"Average (authority owner) delete name processing time in microseconds")
            DURATION_COUNTER( 3, L"NO Create Name avg. duration (us)", L"Average (name owner) create name processing time in microseconds")
            DURATION_COUNTER( 4, L"NO Delete Name avg. duration (us)", L"Average (name owner) delete name processing time in microseconds")
            DURATION_COUNTER( 5, L"Name Exists avg. duration (us)", L"Average name exists processing time in microseconds")
            DURATION_COUNTER( 6, L"AO Create Service avg. duration (us)", L"Average (authority owner) create service processing time in microseconds")
            DURATION_COUNTER( 7, L"AO Update Service avg. duration (us)", L"Average (authority owner) update service processing time in microseconds")
            DURATION_COUNTER( 8, L"AO Delete Service avg. duration (us)", L"Average (authority owner) delete service processing time in microseconds")
            DURATION_COUNTER( 9, L"NO Create Service avg. duration (us)", L"Average (name owner) create service processing time in microseconds")
            DURATION_COUNTER( 10, L"NO Update Service avg. duration (us)", L"Average (name owner) update service processing time in microseconds")
            DURATION_COUNTER( 11, L"NO Delete Service avg. duration (us)", L"Average (name owner) delete service processing time in microseconds")
            DURATION_COUNTER( 12, L"Enumerate Names avg. duration (us)", L"Average enumerate names processing time in microseconds")
            DURATION_COUNTER( 13, L"Enumerate Properties avg. duration (us)", L"Average enumerate properties processing time in microseconds")
            DURATION_COUNTER( 14, L"Property Batch avg. duration (us)", L"Incoming property batch per second")
            DURATION_COUNTER( 15, L"Get Service Description avg. duration (us)", L"Average get service description processing time in microseconds")
            DURATION_COUNTER( 16, L"Prefix Resolve avg. duration (us)", L"Average prefix resolution processing time in microseconds")
            DURATION_COUNTER( 17, L"Unrecognized Operation avg. duration (us)", L"Incoming unrecognized operation processing time in microseconds")

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
            DEFINE_RATE_COUNTER( 1, RateOfAOCreateName )
            DEFINE_RATE_COUNTER( 2, RateOfAODeleteName )
            DEFINE_RATE_COUNTER( 3, RateOfNOCreateName )
            DEFINE_RATE_COUNTER( 4, RateOfNODeleteName )
            DEFINE_RATE_COUNTER( 5, RateOfNameExists )
            DEFINE_RATE_COUNTER( 6, RateOfAOCreateService )
            DEFINE_RATE_COUNTER( 7, RateOfAOUpdateService )
            DEFINE_RATE_COUNTER( 8, RateOfAODeleteService )
            DEFINE_RATE_COUNTER( 9, RateOfNOCreateService )
            DEFINE_RATE_COUNTER( 10, RateOfNOUpdateService )
            DEFINE_RATE_COUNTER( 11, RateOfNODeleteService )
            DEFINE_RATE_COUNTER( 12, RateOfEnumerateNames )
            DEFINE_RATE_COUNTER( 13, RateOfEnumerateProperties )
            DEFINE_RATE_COUNTER( 14, RateOfPropertyBatch )
            DEFINE_RATE_COUNTER( 15, RateOfGetServiceDescription )
            DEFINE_RATE_COUNTER( 16, RateOfPrefixResolve )
            DEFINE_RATE_COUNTER( 17, RateOfInvalidOperation )

            DEFINE_AVG_BASE_COUNTER( 1, DurationOfAOCreateNameBase )
            DEFINE_AVG_BASE_COUNTER( 2, DurationOfAODeleteNameBase )
            DEFINE_AVG_BASE_COUNTER( 3, DurationOfNOCreateNameBase )
            DEFINE_AVG_BASE_COUNTER( 4, DurationOfNODeleteNameBase )
            DEFINE_AVG_BASE_COUNTER( 5, DurationOfNameExistsBase )
            DEFINE_AVG_BASE_COUNTER( 6, DurationOfAOCreateServiceBase )
            DEFINE_AVG_BASE_COUNTER( 7, DurationOfAOUpdateServiceBase )
            DEFINE_AVG_BASE_COUNTER( 8, DurationOfAODeleteServiceBase )
            DEFINE_AVG_BASE_COUNTER( 9, DurationOfNOCreateServiceBase )
            DEFINE_AVG_BASE_COUNTER( 10, DurationOfNOUpdateServiceBase )
            DEFINE_AVG_BASE_COUNTER( 11, DurationOfNODeleteServiceBase )
            DEFINE_AVG_BASE_COUNTER( 12, DurationOfEnumerateNamesBase )
            DEFINE_AVG_BASE_COUNTER( 13, DurationOfEnumeratePropertiesBase )
            DEFINE_AVG_BASE_COUNTER( 14, DurationOfPropertyBatchBase )
            DEFINE_AVG_BASE_COUNTER( 15, DurationOfGetServiceDescriptionBase )
            DEFINE_AVG_BASE_COUNTER( 16, DurationOfPrefixResolveBase )
            DEFINE_AVG_BASE_COUNTER( 17, DurationOfInvalidOperationBase )

            DEFINE_DURATION_COUNTER( 1, DurationOfAOCreateName )
            DEFINE_DURATION_COUNTER( 2, DurationOfAODeleteName )
            DEFINE_DURATION_COUNTER( 3, DurationOfNOCreateName )
            DEFINE_DURATION_COUNTER( 4, DurationOfNODeleteName )
            DEFINE_DURATION_COUNTER( 5, DurationOfNameExists )
            DEFINE_DURATION_COUNTER( 6, DurationOfAOCreateService )
            DEFINE_DURATION_COUNTER( 7, DurationOfAOUpdateService )
            DEFINE_DURATION_COUNTER( 8, DurationOfAODeleteService )
            DEFINE_DURATION_COUNTER( 9, DurationOfNOCreateService )
            DEFINE_DURATION_COUNTER( 10, DurationOfNOUpdateService )
            DEFINE_DURATION_COUNTER( 11, DurationOfNODeleteService )
            DEFINE_DURATION_COUNTER( 12, DurationOfEnumerateNames )
            DEFINE_DURATION_COUNTER( 13, DurationOfEnumerateProperties )
            DEFINE_DURATION_COUNTER( 14, DurationOfPropertyBatch )
            DEFINE_DURATION_COUNTER( 15, DurationOfGetServiceDescription )
            DEFINE_DURATION_COUNTER( 16, DurationOfPrefixResolve )
            DEFINE_DURATION_COUNTER( 17, DurationOfInvalidOperation )
        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<NamingPerformanceCounters> PerformanceCountersSPtr;
}
