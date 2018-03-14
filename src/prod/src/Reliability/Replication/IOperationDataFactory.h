// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        static const GUID IID_IOperationDataFactory =
            {0x01af2d2f,0xe878,0x4db2,{0x89,0xec,0x5f,0x6a,0xfe,0xc2,0xc5,0x6f}};
    
        struct IOperationDataFactory : public IUnknown
        {
            virtual HRESULT STDMETHODCALLTYPE CreateOperationData(
                __in ULONG * segmentSizes,
                __in ULONG segmentSizesCount,
                __out ::IFabricOperationData ** operationData) = 0;
        };
    }
}
