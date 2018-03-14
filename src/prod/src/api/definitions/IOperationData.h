// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IOperationData
    {
        Common::ErrorCode GetData(
            __out ULONG & count, 
            __out const FABRIC_OPERATION_DATA_BUFFER ** buffers);
    };
}
