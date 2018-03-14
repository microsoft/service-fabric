// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IPropertyBatchResult
    {
    public:
        virtual ~IPropertyBatchResult() {};

        virtual ULONG GetFailedOperationIndexInRequest() = 0;

        virtual Common::ErrorCode GetError() = 0;

        virtual Common::ErrorCode GetProperty(
            __in ULONG operationIndexInRequest, 
            __out Naming::NamePropertyResult &) = 0;
    };
}
