// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Accumulator.h"
#include "ServiceModel/federation/NodeId.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class AccumulatorWithMinMax : public Accumulator
        {
        public:
            AccumulatorWithMinMax(bool usePercentages = false);
            AccumulatorWithMinMax(AccumulatorWithMinMax const& other) = default;
            AccumulatorWithMinMax & operator = (AccumulatorWithMinMax const & other) = default;
            
            virtual ~AccumulatorWithMinMax();

            __declspec (property(get = get_Min)) int64 Min;
            int64 get_Min() const;

            __declspec (property(get = get_MinNode)) Federation::NodeId MinNode;
            Federation::NodeId get_MinNode() const;

            __declspec (property(get = get_MaxNode)) Federation::NodeId MaxNode;
            Federation::NodeId get_MaxNode() const;

            // Is current minimum value valid?
            __declspec (property(get = get_IsMinValid)) bool IsMinValid;
            bool get_IsMinValid() const { return minValid_; }

            __declspec (property(get = get_Max)) int64 Max;
            int64 get_Max() const;

            // Is current maximum value valid?
            __declspec (property(get = get_IsMaxValid)) bool IsMaxValid;
            bool get_IsMaxValid() const { return maxValid_; }

            // Are current maximum and minimum values valid?
            __declspec (property(get = get_IsValid)) bool IsValid;
            bool get_IsValid() const { return maxValid_ && minValid_; }

            __declspec (property(get = get_EmptyNodeCount)) size_t EmptyNodeCount;
            size_t get_EmptyNodeCount() const { return emptyNodeCount_; }

            __declspec (property(get = get_NonEmptyAverageLoad)) double NonEmptyAverageLoad;
            double get_NonEmptyAverageLoad() const { return (nonEmptyNodeCount_ > 0)? ((double)nonEmptyNodeLoad_ / nonEmptyNodeCount_) : 0.0; }

            __declspec (property(get = get_NonEmptyNodeCount)) size_t NonEmptyNodeCount;
            size_t get_NonEmptyNodeCount() const { return nonEmptyNodeCount_; }

            __declspec (property(get = get_NonEmptyNodeLoad)) int64 NonEmptyNodeLoad;
            int64 get_NonEmptyNodeLoad() const { return nonEmptyNodeLoad_; }

            __declspec (property(get = get_NonBeneficialMin)) int64 NonBeneficialMin;
            int64 get_NonBeneficialMin() const { return nonBeneficialMin_; }

            __declspec (property(get = get_NonBeneficialMax)) int64 NonBeneficialMax;
            int64 get_NonBeneficialMax() const { return nonBeneficialMax_; }

            __declspec (property(get = get_NonBeneficialCount)) size_t NonBeneficialCount;
            size_t get_NonBeneficialCount() const { return nonBeneficialCount_; }

            virtual void Clear();
            virtual void AddOneValue(int64 value, int64 capacity, Federation::NodeId const & nodeId);
            virtual void AdjustOneValue(int64 oldValue, int64 newValue, int64 capacity, Federation::NodeId const & nodeId);
            virtual void AddEmptyNodes(size_t nodeCount);
            virtual void AddNonEmptyLoad(int64 val);
            virtual void AddNonBeneficialLoad(int64 val);

        private:
            int64 min_;
            bool minValid_;
            int64 max_;
            bool maxValid_;
            Federation::NodeId minNode_;
            Federation::NodeId maxNode_;
            size_t emptyNodeCount_;
            size_t nonEmptyNodeCount_;
            int64 nonEmptyNodeLoad_;
            int64 nonBeneficialMin_;
            int64 nonBeneficialMax_;
            size_t nonBeneficialCount_;
        };
    }
}
