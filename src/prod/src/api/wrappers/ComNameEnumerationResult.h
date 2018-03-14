// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // d8f52922-8dac-4448-a6c3-6678f5b96e79
    static const GUID CLSID_ComNameEnumerationResult =
    {0xd8f52922, 0x8dac, 0x4448,{0xa6, 0xc3, 0x66, 0x78, 0xf5, 0xb9, 0x6e, 0x79}};

    class ComNameEnumerationResult :
        public IFabricNameEnumerationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComNameEnumerationResult)

        COM_INTERFACE_LIST2(
            ComNameEnumerationResult,
            IID_IFabricNameEnumerationResult,
            IFabricNameEnumerationResult,
            CLSID_ComNameEnumerationResult,
            ComNameEnumerationResult)

    public:
        ComNameEnumerationResult(Naming::EnumerateSubNamesResult && subNames);

        Naming::EnumerateSubNamesToken const & get_ContinuationToken();

        FABRIC_ENUMERATION_STATUS STDMETHODCALLTYPE get_EnumerationStatus(void);

        HRESULT STDMETHODCALLTYPE GetNames(
            __out ULONG * itemCount,
            __out FABRIC_URI const ** bufferedItems);

    private:
        Naming::EnumerateSubNamesResult enumerationResult_;
        std::vector<LPCWSTR> buffer_;
    };
}
