// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;

    struct ModuleSetup
    {
        ModuleSetup()
        {
            // load tracing configuration
            Config cfg;
            TraceProvider::LoadConfiguration(cfg);
        }
    };

    BOOST_GLOBAL_FIXTURE(ModuleSetup);
}
