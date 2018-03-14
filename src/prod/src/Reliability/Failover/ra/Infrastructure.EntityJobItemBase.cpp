// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

EntityJobItemBase::EntityJobItemBase(
    EntityEntryBaseSPtr && entry, 
    std::wstring const & activityId, 
    JobItemCheck::Enum checks, 
    JobItemDescription const & traceParameters, 
    MultipleEntityWorkSPtr const & work)
    : entry_(move(entry)),
    activityId_(activityId),
    isStarted_(false),
    isCommitted_(false),
    wasDropped_(false),
    description_(traceParameters),
    checks_(checks),
    work_(work)
{
    ASSERT_IF(entry_ == nullptr, "Entry cannot be null");
    ASSERT_IF(activityId_.empty(), "ActivityId must be present");
}

EntityJobItemBase::~EntityJobItemBase()
{
}
