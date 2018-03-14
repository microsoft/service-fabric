// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

PerformanceCounterSetDefinition::PerformanceCounterSetDefinition(Guid const & identifier, std::wstring const & name, std::wstring const & description, PerformanceCounterSetInstanceType::Enum instanceType) :
    identifier_(identifier),
    name_(name),
    description_(description),
    counterSetInstanceType_(instanceType)
{
}

PerformanceCounterSetDefinition::PerformanceCounterSetDefinition(PerformanceCounterSetDefinition const & other) :
    identifier_(other.identifier_),
    name_(other.name_),
    description_(other.description_),
    counterSetInstanceType_(other.counterSetInstanceType_),
    counterDefinitions_(other.counterDefinitions_)
{
}

PerformanceCounterSetDefinition::PerformanceCounterSetDefinition(PerformanceCounterSetDefinition && other) :
    identifier_(std::move(other.identifier_)),
    name_(std::move(other.name_)),
    description_(std::move(other.description_)),
    counterSetInstanceType_(std::move(other.counterSetInstanceType_)),
    counterDefinitions_(std::move(other.counterDefinitions_))
{
}

void PerformanceCounterSetDefinition::AddCounterDefinition(PerformanceCounterDefinition const & PerformanceCounterDefinition)
{
    auto it = counterDefinitions_.find(PerformanceCounterDefinition.Identifier);

    ASSERT_IF(end(counterDefinitions_) != it, "Counter already exits");

    counterDefinitions_.insert(std::make_pair(PerformanceCounterDefinition.Identifier, PerformanceCounterDefinition));
}

void PerformanceCounterSetDefinition::AddCounterDefinition(PerformanceCounterDefinition && PerformanceCounterDefinition)
{
    auto it = counterDefinitions_.find(PerformanceCounterDefinition.Identifier);

    ASSERT_IF(end(counterDefinitions_) != it, "Counter already exits");

    counterDefinitions_.insert(std::make_pair(PerformanceCounterDefinition.Identifier, std::move(PerformanceCounterDefinition)));
}

PerformanceCounterSetDefinition const & PerformanceCounterSetDefinition::operator = (PerformanceCounterSetDefinition const & other)
{
    identifier_ = other.identifier_;
    name_ = other.name_;
    description_ = other.description_;
    counterSetInstanceType_ = other.counterSetInstanceType_;
    counterDefinitions_ = other.counterDefinitions_;

    return *this;
}

PerformanceCounterSetDefinition const & PerformanceCounterSetDefinition::operator = (PerformanceCounterSetDefinition && other)
{
    identifier_ = std::move(other.identifier_);
    name_ = std::move(other.name_);
    description_ = std::move(other.description_);
    counterSetInstanceType_ = std::move(other.counterSetInstanceType_);
    counterDefinitions_ = std::move(other.counterDefinitions_);

    return *this;
}

PerformanceCounterSetDefinition::~PerformanceCounterSetDefinition(void)
{
}
