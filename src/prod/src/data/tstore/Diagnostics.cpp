// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;
using namespace Data::Utilities;

void Diagnostics::Validate(NTSTATUS status)
{
    THROW_ON_FAILURE(status);
}

void Diagnostics::GetExceptionStackTrace(
    __in ktl::Exception const & exception, 
    __out KDynStringA& stackString)
{
    auto exc = const_cast<ktl::Exception *>(&exception);

    // Not asserting on return value in case ToString is called after KTL deinitialization
    // In that case, stackString will remain empty
    exc->ToString(stackString);

}
