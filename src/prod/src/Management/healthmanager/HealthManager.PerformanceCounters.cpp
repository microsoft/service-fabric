// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management::HealthManager;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_COUNTER_SET(HealthManagerCounters)

void HealthManagerCounters::OnHealthReportsReceived(size_t count)
{
    auto increment = static_cast<PerformanceCounterValue>(count);
    this->NumberOfReceivedHealthReports.IncrementBy(increment);
    this->RateOfReceivedHealthReports.IncrementBy(increment);
}

void HealthManagerCounters::OnSuccessfulHealthReport(int64 startTime)
{
    this->NumberOfSuccessfulHealthReports.Increment();
    this->RateOfSuccessfulHealthReports.Increment();
    this->AverageReportProcessTime.Update(startTime);
}

void HealthManagerCounters::OnHealthQueryReceived()
{
    this->NumberOfReceivedHealthQueries.Increment();
    this->RateOfReceivedHealthQueries.Increment();
}

void HealthManagerCounters::OnHealthQueryDropped()
{
    this->NumberOfDroppedHealthQueries.Increment();
    this->RateOfDroppedHealthQueries.Increment();
}

void HealthManagerCounters::OnSuccessfulHealthQuery(int64 startTime)
{
    this->RateOfSuccessfulHealthQueries.Increment();
    this->AverageQueryProcessTime.Update(startTime);
}

void HealthManagerCounters::OnEvaluateClusterHealthCompleted(int64 startTime)
{
    this->AverageClusterHealthEvaluationTime.Update(startTime);
}

void HealthManagerCounters::OnRequestContextTryAcceptCompleted(RequestContext const & context)
{
    if (context.IsAcceptedForProcessing)
    {
        this->OnRequestContextAcceptedForProcessing(context);
    }
    else
    {
        this->OnRequestContextDropped(context);
    }
}

void HealthManagerCounters::OnRequestContextAcceptedForProcessing(RequestContext const & context)
{
    auto contextSize = static_cast<PerformanceCounterValue>(context.GetEstimatedSize());
    if (context.Priority == Priority::Critical)
    {
        this->NumberOfQueuedCriticalReports.Increment();
        this->SizeOfQueuedCriticalReports.IncrementBy(contextSize);
    }
    else
    {
        this->NumberOfQueuedReports.Increment();
        this->SizeOfQueuedReports.IncrementBy(contextSize);
    }
}

void HealthManagerCounters::OnRequestContextDropped(RequestContext const & context)
{
    if (context.Priority == Priority::Critical)
    {
        this->NumberOfDroppedCriticalHealthReports.Increment();
    }
    else
    {
        this->NumberOfDroppedHealthReports.Increment();
        this->RateOfDroppedHealthReports.Increment();
    }
}

void HealthManagerCounters::OnRequestContextCompleted(RequestContext const & context)
{
    if (context.IsAcceptedForProcessing)
    {
        auto negativeContextSize = -(static_cast<PerformanceCounterValue>(context.GetEstimatedSize()));
        if (context.Priority == Priority::Critical)
        {
            this->NumberOfQueuedCriticalReports.Decrement();
            this->SizeOfQueuedReports.IncrementBy(negativeContextSize);
        }
        else
        {
            this->NumberOfQueuedReports.Decrement();
            this->SizeOfQueuedReports.IncrementBy(negativeContextSize);
        }
    }
}
