// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class IFailPointAction
    {
    public:
        virtual bool Invoke(FailPointContext context) =0;
        virtual bool CheckResult()=0;
        virtual std::wstring ToString()=0;
    };
}
