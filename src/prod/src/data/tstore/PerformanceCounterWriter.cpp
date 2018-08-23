// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;

PerformanceCounterWriter::PerformanceCounterWriter(__in Common::PerformanceCounterData * counterData)
    : counterData_(counterData)
{

}

bool PerformanceCounterWriter::IsEnabled()
{
    return counterData_ != nullptr;
}
