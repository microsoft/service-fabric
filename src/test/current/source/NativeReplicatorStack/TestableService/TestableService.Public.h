// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include <atomic>
#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../../prod/src/data/utilities/Data.Utilities.Public.h"

namespace TestableService
{
    using ::_delete;
}

#define PROPERTY(TYPE, var, name)\
    _declspec(property(get = get_##name, put = set_##name)) TYPE name;\
    TYPE& get_##name()\
    {\
        return var;\
    }\
    TYPE get_##name() const\
    {\
        return var;\
    }\
    void set_##name(const TYPE& val)\
    {\
        var = val;\
    }

#include "WorkStatusEnum.h"
#include "TestableServiceProgress.h"
#include "ITestableService.h"
#include "TestableServiceBase.h"

#undef PROPERTY
