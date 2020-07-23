// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseError
    {
    public:
        static Common::ErrorCode GetErrorCode(JET_ERR);

    private:
        EseError() { }

        static Common::ErrorCode GetErrorCodeWithMessage(JET_ERR, Common::ErrorCodeValue::Enum); 
    };
}
