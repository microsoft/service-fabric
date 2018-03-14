// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AccumulatorWithMinMax.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

AccumulatorWithMinMax::AccumulatorWithMinMax()
    : Accumulator(),
    min_(0),
    minValid_(false),
    max_(0),
    maxValid_(false),
    minNode_(Federation::NodeId::MinNodeId),
    maxNode_(Federation::NodeId::MinNodeId),
    emptyNodeCount_(0),
    nonEmptyNodeCount_(0),
    nonEmptyNodeLoad_(0),
    nonBeneficialMin_(0),
    nonBeneficialMax_(0),
    nonBeneficialCount_(0)
{
}

AccumulatorWithMinMax::AccumulatorWithMinMax(AccumulatorWithMinMax const& other)
    : Accumulator(other),
    min_(other.min_),
    minValid_(other.minValid_),
    max_(other.max_),
    maxValid_(other.maxValid_),
    minNode_(other.minNode_),
    maxNode_(other.maxNode_),
    emptyNodeCount_(other.emptyNodeCount_),
    nonEmptyNodeCount_(other.nonEmptyNodeCount_),
    nonEmptyNodeLoad_(other.nonEmptyNodeLoad_),
    nonBeneficialMin_(other.nonBeneficialMin_),
    nonBeneficialMax_(other.nonBeneficialMax_),
    nonBeneficialCount_(other.nonBeneficialCount_)
{
}

AccumulatorWithMinMax::~AccumulatorWithMinMax()
{
}

void AccumulatorWithMinMax::Clear()
{
    Accumulator::Clear();
    min_ = 0;
    minValid_ = false;
    max_ = 0;
    maxValid_ = false;
    minNode_ = Federation::NodeId::MinNodeId;
    maxNode_ = Federation::NodeId::MinNodeId;
    emptyNodeCount_ = 0;
    nonEmptyNodeCount_ = 0;
    nonEmptyNodeLoad_ = 0;
    nonBeneficialMin_ = 0;
    nonBeneficialMax_ = 0;
    nonBeneficialCount_ = 0;
}

AccumulatorWithMinMax & AccumulatorWithMinMax::operator= (AccumulatorWithMinMax const & other)
{
    if (this != &other)
    {
        Accumulator::operator=(other);
        min_ = other.min_;
        minValid_ = other.minValid_;
        max_ = other.max_;
        maxValid_ = other.maxValid_;
        minNode_ = other.minNode_;
        maxNode_ = other.maxNode_;
        emptyNodeCount_ = other.emptyNodeCount_;
        nonEmptyNodeCount_ = other.nonEmptyNodeCount_;
        nonEmptyNodeLoad_ = other.nonEmptyNodeLoad_;
        nonBeneficialMin_ = other.nonBeneficialMin_;
        nonBeneficialMax_ = other.nonBeneficialMax_;
        nonBeneficialCount_ = other.nonBeneficialCount_;
    }

    return *this;
}

void AccumulatorWithMinMax::AddOneValue(int64 value, Federation::NodeId const & nodeId)
{
    if (IsEmpty)
    {
        min_ = value;
        max_ = value;
        minValid_ = true;
        maxValid_ = true;
        minNode_ = nodeId;
        maxNode_ = nodeId;
    }
    else
    {
        if (minValid_ && value < min_)
        {
            min_ = value;
            minNode_ = nodeId;
        }

        if (maxValid_ && value > max_)
        {
            max_ = value;
            maxNode_ = nodeId;
        }
    }

    Accumulator::AddOneValue(value);
}

void AccumulatorWithMinMax::AdjustOneValue(int64 oldValue, int64 newValue, Federation::NodeId const & nodeId)
{
    ASSERT_IF(maxValid_ && oldValue > max_, "Error: old value {0}, max {1}", oldValue, max_);
    ASSERT_IF(minValid_ && oldValue < min_, "Error: old value {0}, min {1}", oldValue, min_);

    if (oldValue == newValue)
    {
        return;
    }

    if (minValid_)
    {
        if (newValue < min_)
        {
            min_ = newValue;
            minNode_ = nodeId;
        }
        else if (min_ == oldValue)
        {
            minValid_ = false;
        }
    }

    if (maxValid_)
    {
        if (newValue > max_)
        {
            max_ = newValue;
            maxNode_ = nodeId;
        }
        else if (max_ == oldValue)
        {
            maxValid_ = false;
        }
    }

    Accumulator::AdjustOneValue(oldValue, newValue);
}

int64 AccumulatorWithMinMax::get_Min() const
{
    ASSERT_IFNOT(minValid_, "Min value is not valid");
    return min_;
}

int64 AccumulatorWithMinMax::get_Max() const
{
    ASSERT_IFNOT(maxValid_, "Max value is not valid");
    return max_;
}

Federation::NodeId AccumulatorWithMinMax::get_MinNode() const
{
    ASSERT_IFNOT(minValid_, "Min value is not valid");
    return minNode_;
}

Federation::NodeId AccumulatorWithMinMax::get_MaxNode() const
{
    ASSERT_IFNOT(minValid_, "Max value is not valid");
    return maxNode_;
}

void AccumulatorWithMinMax::AddEmptyNodes(size_t nodeCount){
    emptyNodeCount_ += nodeCount;
}

void AccumulatorWithMinMax::AddNonEmptyLoad(int64 val)
{
    ++nonEmptyNodeCount_;
    nonEmptyNodeLoad_ += val;
}

void AccumulatorWithMinMax::AddNonBeneficialLoad(int64 val)
{
    if (nonBeneficialCount_ == 0)
    {
        nonBeneficialMin_ = val;
        nonBeneficialMax_ = val;
    }
    else
    {
        if (val < nonBeneficialMin_)
        {
            nonBeneficialMin_ = val;
        }

        if (val > nonBeneficialMax_)
        {
            nonBeneficialMax_ = val;
        }
    }

    ++nonBeneficialCount_;
}
