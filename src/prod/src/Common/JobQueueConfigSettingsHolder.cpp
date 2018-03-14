// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

JobQueueConfigSettingsHolder::JobQueueConfigSettingsHolder()
{
}

JobQueueConfigSettingsHolder::~JobQueueConfigSettingsHolder()
{
}

JobQueueConfigSettingsHolder::JobQueueConfigSettingsHolder(JobQueueConfigSettingsHolder && other)
{
    UNREFERENCED_PARAMETER(other);
}

JobQueueConfigSettingsHolder & JobQueueConfigSettingsHolder::operator=(JobQueueConfigSettingsHolder && other)
{
    UNREFERENCED_PARAMETER(other);
    return *this;
}


int JobQueueConfigSettingsHolder::get_MaxThreadCount() const
{
    int value = this->MaxThreadCountValue;
    return value == 0 ? static_cast<int>(Environment::GetNumberOfProcessors()) : value;
}
