// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

UpgradeDescription::UpgradeDescription(ApplicationUpgradeSpecification && specification)
    : specification_(move(specification))
{
}

UpgradeDescription::UpgradeDescription(UpgradeDescription && other)
    : specification_(move(other.specification_))
{
}

UpgradeDescription & UpgradeDescription::operator = (UpgradeDescription && other)
{
    if (this != &other)
    {
        specification_ = move(other.specification_);
    }

    return *this;
}

void UpgradeDescription::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write(specification_);
}

void UpgradeDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->UpgradeDescription(contextSequenceId, specification_);
}
