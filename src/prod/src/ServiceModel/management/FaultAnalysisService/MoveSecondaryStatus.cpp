// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("MoveSecondaryStatus");

MoveSecondaryStatus::MoveSecondaryStatus()
: secondaryMoveResultSPtr_()
{
}

MoveSecondaryStatus::MoveSecondaryStatus(
    shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> && secondaryMoveResultSPtr)
    : secondaryMoveResultSPtr_(std::move(secondaryMoveResultSPtr))
{
}

MoveSecondaryStatus::MoveSecondaryStatus(MoveSecondaryStatus && other)
: secondaryMoveResultSPtr_(move(other.secondaryMoveResultSPtr_))
{
}

MoveSecondaryStatus & MoveSecondaryStatus::operator=(MoveSecondaryStatus && other)
{
    if (this != &other)
    {
        secondaryMoveResultSPtr_ = move(other.secondaryMoveResultSPtr_);
    }

    return *this;
}

shared_ptr<SecondaryMoveResult> const & MoveSecondaryStatus::GetMoveSecondaryResult()
{
    return secondaryMoveResultSPtr_;
}
