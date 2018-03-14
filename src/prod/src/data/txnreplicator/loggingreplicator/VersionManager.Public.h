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
    namespace LoggingReplicator
    {
        using ::_delete;
    }
}

// Version Manager Factory is a test class that is being exposed tp enable upper layer components to be tested.
// DO NOT USE IT IN PRODUCTION.
#include "VersionManagerFactory.h"
