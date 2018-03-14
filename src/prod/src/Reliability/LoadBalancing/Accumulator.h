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
            Accumulator();
            Accumulator(Accumulator const& other);

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

            virtual void Clear();
            virtual void AddOneValue(int64 value);
            virtual void AdjustOneValue(int64 oldValue, int64 newValue);

            virtual Accumulator & operator = (Accumulator const & other);

        private:
            size_t count_;
            double sum_;
            double squaredSum_;
        };
    }
}
