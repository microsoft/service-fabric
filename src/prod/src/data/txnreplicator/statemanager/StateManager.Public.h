// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#include <ktl.h>
#include <KTpl.h> 

namespace Data
{
    namespace StateManager
    {
        using ::_delete;
    }
}

// Interfaces
#include "FactoryArguments.h"
#include "IStateProvider2Factory.h"
#include "IStateSerializer.h"
