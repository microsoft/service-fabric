// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class RetryableErrorStateThresholdCollection
        {
        public:
            typedef std::function<int(Reliability::FailoverConfig const &)> DynamicThresholdReadFunctionPtr;
            
            struct Threshold
            {
                int WarningThreshold;
                int DropThreshold;
                int RestartThreshold;
                int ErrorThreshold;
            };

            RetryableErrorStateThresholdCollection();

            Threshold GetThreshold(RetryableErrorStateName::Enum state, FailoverConfig const & config) const;

            void AddThreshold(
                RetryableErrorStateName::Enum state, 
                int warningThreshold, 
                int dropThreshold,
                int restartThreshold,
                int errorThreshold);

            void AddThreshold(
                RetryableErrorStateName::Enum state,
                DynamicThresholdReadFunctionPtr WarningThresholdReader,
                DynamicThresholdReadFunctionPtr DropThresholdReader,
                DynamicThresholdReadFunctionPtr restartThresholdReader,
                DynamicThresholdReadFunctionPtr errorThresholdReader);

            static RetryableErrorStateThresholdCollection CreateDefault();

        private:
            struct Storage
            {
                DynamicThresholdReadFunctionPtr WarningThresholdReader;
                DynamicThresholdReadFunctionPtr DropThresholdReader;
                DynamicThresholdReadFunctionPtr RestartThresholdReader;
                DynamicThresholdReadFunctionPtr ErrorThresholdReader;
            };

            typedef std::map<RetryableErrorStateName::Enum, Storage> StorageMap;

            void Insert(RetryableErrorStateName::Enum state, Storage s);

            StorageMap thresholds_;
        };
    }
}


