// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("MovePrimaryStatus");

MovePrimaryStatus::MovePrimaryStatus()
: primaryMoveResultSPtr_()
{
}

MovePrimaryStatus::MovePrimaryStatus(
    shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> && primaryMoveResultSPtr)
    : primaryMoveResultSPtr_(std::move(primaryMoveResultSPtr))
{
}

MovePrimaryStatus::MovePrimaryStatus(MovePrimaryStatus && other)
: primaryMoveResultSPtr_(move(other.primaryMoveResultSPtr_))
{
}

MovePrimaryStatus & MovePrimaryStatus::operator=(MovePrimaryStatus && other)
{
    if (this != &other)
    {
        primaryMoveResultSPtr_ = move(other.primaryMoveResultSPtr_);
    }

    return *this;
}

shared_ptr<PrimaryMoveResult> const & MovePrimaryStatus::GetMovePrimaryResult()
{
    return primaryMoveResultSPtr_;
}
