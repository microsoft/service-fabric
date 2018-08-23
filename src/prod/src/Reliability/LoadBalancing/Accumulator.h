// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Accumulator
        {
        public:
            explicit Accumulator(bool usePercentages);
            Accumulator(Accumulator const& other) = default;
            Accumulator & operator = (Accumulator const & other) = default;

            virtual ~Accumulator();

            __declspec (property(get=get_Count)) size_t Count;
            size_t get_Count() const { return count_; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return count_ == 0; }

            __declspec (property(get=get_Sum)) double Sum;
            double get_Sum() const { return sum_; }

            __declspec (property(get=get_SquaredSum)) double SquaredSum;
            double get_SquaredSum() const { return squaredSum_; }

            __declspec (property(get=get_Average)) double Average;
            double get_Average() const { return sum_ / count_; }

            __declspec (property(get=get_NormStdDev)) double NormStdDev;
            double get_NormStdDev() const;

            __declspec (property(get = get_UsePercentage)) bool UsePercentage;
            bool get_UsePercentage() const { return usePercentage_; }

            __declspec (property(get = get_AbsoluteSum)) double AbsoluteSum;
            double get_AbsoluteSum() const { return absoluteSum_; }

            __declspec (property(get = get_CapacitySum)) double CapacitySum;
            double get_CapacitySum() const { return capacitySum_; }

            virtual void Clear();
            virtual void AddOneValue(int64 value, int64 capacity);
            virtual void AdjustOneValue(int64 oldValue, int64 newValue, int64 capacity);

        private:
            // Number of values in accumulator
            size_t count_;
            // Sum of values in accumulator (values can be percents)
            double sum_;
            // Squared sum of values in accumulator (values can be percents)
            double squaredSum_;
            // Shall accumulator contain percents or absolute values
            bool usePercentage_;
            // Sum of absolute values (same as sum_ if usePercentage_ is false)
            double absoluteSum_;
            // Sum of all capacities
            double capacitySum_;
        };
    }
}
