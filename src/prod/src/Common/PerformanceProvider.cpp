// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PerformanceProvider.h"

using namespace Common;

PerformanceProvider::PerformanceProvider(Guid const & providerId) :
    providerId_(providerId),
    providerHandle_(NULL)
{
    auto error = ::PerfStartProvider((LPGUID)&(providerId_.AsGUID()), NULL, &providerHandle_);

    TESTASSERT_IF(ERROR_SUCCESS != error, "PerfStartProvider failed");
}

PerformanceProvider::~PerformanceProvider(void)
{
    if (NULL != providerHandle_)
    {
        ::PerfStopProvider(providerHandle_);
        providerHandle_ = NULL;
    }
}
