// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

PerformanceCounterDefinition::PerformanceCounterDefinition(PerformanceCounterId identifier, PerformanceCounterType::Enum type, std::wstring const & name, std::wstring const & description) :
    identifier_(identifier),
    baseIdentifier_((PerformanceCounterId)(-1)),
    counterType_(type),
    name_(name),
    description_(description)
{
}

PerformanceCounterDefinition::PerformanceCounterDefinition(PerformanceCounterId identifier, PerformanceCounterId baseIdentifier, PerformanceCounterType::Enum type, std::wstring const & name, std::wstring const & description) :
    identifier_(identifier),
    baseIdentifier_(baseIdentifier),
    counterType_(type),
    name_(name),
    description_(description)
{
}

PerformanceCounterDefinition::PerformanceCounterDefinition(PerformanceCounterDefinition const & other) :
    identifier_(other.identifier_),
    baseIdentifier_(other.baseIdentifier_),
    counterType_(other.counterType_),
    name_(other.name_),
    description_(other.description_)
{
}

PerformanceCounterDefinition::PerformanceCounterDefinition(PerformanceCounterDefinition && other)  :
    identifier_(std::move(other.identifier_)),
    baseIdentifier_(std::move(other.baseIdentifier_)),
    counterType_(std::move(other.counterType_)),
    name_(std::move(other.name_)),
    description_(std::move(other.description_))
{
}

PerformanceCounterDefinition const & PerformanceCounterDefinition::operator = (PerformanceCounterDefinition const & other)
{
    identifier_ = other.identifier_;
    name_ = other.name_;
    description_ = other.description_;
    baseIdentifier_ = other.baseIdentifier_;
    counterType_ = other.counterType_;
    
    return *this;
}

PerformanceCounterDefinition const & PerformanceCounterDefinition::operator = (PerformanceCounterDefinition && other)
{
    identifier_ = std::move(other.identifier_);
    name_ = std::move(other.name_);
    description_ = std::move(other.description_);
    baseIdentifier_ = std::move(other.baseIdentifier_);
    counterType_ = std::move(other.counterType_);

    return *this;
}

PerformanceCounterDefinition::~PerformanceCounterDefinition(void)
{
}
