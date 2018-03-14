// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Class used to wrap configuration settings for job queue.
    // The creator of the job queue points to the configuration where the settings are read from.
    // The reason for the class is to be able to make the configurations dynamic.
    // If passed in the ctor of the job queue and kept inside the queue as variables,
    // they can't be dynamically updated.
    //
    class JobQueueConfigSettingsHolder
    {
        DENY_COPY(JobQueueConfigSettingsHolder)
    public:
        JobQueueConfigSettingsHolder();
        virtual ~JobQueueConfigSettingsHolder();

        JobQueueConfigSettingsHolder(JobQueueConfigSettingsHolder && other);
        JobQueueConfigSettingsHolder & operator=(JobQueueConfigSettingsHolder && other);

        // The maximum size of the queue, which counts items in all states: not started, pending, async ready
        // Default value: no limit
        __declspec(property(get = get_MaxQueueSize)) int MaxQueueSize;
        virtual int get_MaxQueueSize() const { return INT_MAX; }

        // The max parallel work count allowed to start at one time.
        __declspec(property(get = get_MaxParallelPendingWorkCount)) int MaxParallelPendingWorkCount;
        virtual int get_MaxParallelPendingWorkCount() const { return INT_MAX; }

        // Returns MaxThreadCountValue or Number of CPUs (if 0)
        __declspec(property(get = get_MaxThreadCount)) int MaxThreadCount;
        int get_MaxThreadCount() const;

        // If true, emit more tracing to debug queue issues.
        __declspec(property(get = get_TraceProcessingThreads)) bool TraceProcessingThreads;
        virtual bool get_TraceProcessingThreads() const { return false; }

    protected:
        // Default value: number of CPUs
        __declspec(property(get = get_MaxThreadCountValue)) int MaxThreadCountValue;
        virtual int get_MaxThreadCountValue() const { return 0; }
    };

    using JobQueueConfigSettingsHolderUPtr = std::unique_ptr<JobQueueConfigSettingsHolder>;
}
