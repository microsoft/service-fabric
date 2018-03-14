// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // Queryable interface for concrete IFabricPropertyEnumerationResult used internally
    static const GUID CLSID_ComEnumeratePropertiesResult =
        // 3310dedf-6da1-4c69-8378-14f38974e001
        {0x3310dedf, 0x6da1, 0x4c69, {0x83, 0x78, 0x14, 0xf3, 0x89, 0x74, 0xe0, 0x01} };

    class ComEnumeratePropertiesResult :
        public IFabricPropertyEnumerationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComEnumeratePropertiesResult)

        COM_INTERFACE_LIST2(
            ComEnumeratePropertiesResult,
            IID_IFabricPropertyEnumerationResult,
            IFabricPropertyEnumerationResult,
            CLSID_ComEnumeratePropertiesResult,
            ComEnumeratePropertiesResult)

    public:
        explicit ComEnumeratePropertiesResult(Naming::EnumeratePropertiesResult && enumerationResult);

        Naming::EnumeratePropertiesToken const & get_ContinuationToken();

        FABRIC_ENUMERATION_STATUS STDMETHODCALLTYPE get_EnumerationStatus();

        ULONG STDMETHODCALLTYPE get_PropertyCount();

        HRESULT STDMETHODCALLTYPE GetProperty(
            ULONG index,
            __out IFabricPropertyValueResult ** property);

    private:
        Naming::EnumeratePropertiesResult enumerationResult_;
        std::vector<ComNamedPropertyCPtr> comProperties_;
    };

    typedef Common::ComPointer<ComEnumeratePropertiesResult> ComEnumeratePropertiesResultCPtr;
}
