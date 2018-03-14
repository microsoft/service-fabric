// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComNamedPropertyMetadata
        : public IFabricPropertyMetadataResult
        , private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComNamedPropertyMetadata);

        COM_INTERFACE_LIST1(ComNamedPropertyMetadata, IID_IFabricPropertyMetadataResult, IFabricPropertyMetadataResult);

    public:
        ComNamedPropertyMetadata(Naming::NamePropertyMetadataResult const & metadata);
        const FABRIC_NAMED_PROPERTY_METADATA * STDMETHODCALLTYPE get_Metadata(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_NAMED_PROPERTY_METADATA> buffer_;
    };

    typedef Common::ComPointer<ComNamedPropertyMetadata> ComNamedPropertyMetadataCPtr;
}
