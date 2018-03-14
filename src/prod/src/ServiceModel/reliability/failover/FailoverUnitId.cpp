// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

bool FailoverUnitId::get_IsFM() const
{
    return guid_ == ServiceModel::Constants::FMServiceGuid;
}

void FailoverUnitId::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModelEventSource::Trace->FailoverUnitId(
        contextSequenceId,
        guid_);
}
