// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;

namespace FabricTest
{
    void RandomSessionConfig::Initialize()
    {
        TestSession::FailTestIf(RandomVoteCount < 1, "RandomVoteCount cannot be less than 1");
    }
}
