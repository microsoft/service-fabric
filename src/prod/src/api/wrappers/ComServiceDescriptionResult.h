// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComServiceDescriptionResult
        : public IFabricServiceDescriptionResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComServiceDescriptionResult)

            COM_INTERFACE_LIST1(
            ComServiceDescriptionResult,
            IID_IFabricServiceDescriptionResult,
            IFabricServiceDescriptionResult)

    public:
        explicit ComServiceDescriptionResult(Naming::PartitionedServiceDescriptor && nativeResult);

        const FABRIC_SERVICE_DESCRIPTION * STDMETHODCALLTYPE get_Description(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SERVICE_DESCRIPTION> description_;
    };
}
