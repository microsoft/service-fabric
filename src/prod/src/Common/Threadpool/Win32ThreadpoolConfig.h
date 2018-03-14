// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <string>

namespace Threadpool
{
    int GetThreadpoolThrottle();
    void TraceThreadpoolMsg(int level, const std::string &msg);
    void ThreadpoolAssert(const char* msg);
}
