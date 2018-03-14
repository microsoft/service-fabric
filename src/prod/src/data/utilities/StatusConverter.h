// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class StatusConverter
        {
        public:
            // Summary:
            // This test whether the input HRESULT is an SF HRESULT (ERROR_CODE_VALUE).
            // 
            // Returned boolean indicates whether the input HRESULT is a SF HRESULT.
            // TODO: ForceInline and Inline cause build warning LNK4221.
            static bool IsSFHResult(__in HRESULT hResult) noexcept;

            // Summary:
            // This method converts an SF HRESULT (ERROR_CODE_VALUE) into SF STATUS.
            // 
            // Returned boolean indicates whether conversion was successful.
            // TODO: ForceInline and Inline cause build warning LNK4221.
            static bool TryConvertToSFSTATUS(__in HRESULT hResult, __out NTSTATUS & status) noexcept;
            
            // Summary:
            // Same as TryConvertToSFSTATUS, expect that the method asserts if the conversion is not successful
            static NTSTATUS ConvertToSFSTATUS(__in HRESULT hResult) noexcept;

            // Summary:
            // This method converts an HRESULT into NTSTATUS.
            // Unlike TryConvertToSFSTATUS which converts SF HRESULT into NTSTATUS without loss,
            // This method falls back to set conversion dictionary for well known error codes.
            // If the HRESULT is not SF HRESULT and not in the well known dictionary, conversion will fail.
            // 
            // Returned boolean indicates whether conversion was successful.
            // TODO: ForceInline and Inline cause build warning LNK4221.
            static bool TryConvert(__in HRESULT hResult, __out NTSTATUS & status) noexcept;
 
            // Summary:
            // Same as TryConvert, expect that the method asserts if the conversion is not successful
            static NTSTATUS Convert(__in HRESULT hResult) noexcept;

        public: // NTSTATUS to HRESULT
            // Summary:
            // This test whether the a NTSTATUS is an SF STATUS (ERROR_CODE_VALUE).
            // 
            // Returned boolean indicates whether the input HRESULT is a SF HRESULT.
            // TODO: ForceInline and Inline cause build warning LNK4221.
            static bool IsSFStatus(__in NTSTATUS status) noexcept;

            // Summary:
            // This method converts a SF STATUS in to SF HRESULT (ERROR_CODE_VALUE).
            // 
            // Returned boolean indicates whether conversion was successful.
            // TODO: ForceInline and Inline cause build warning LNK4221.
            static bool ToSFHResult(__in NTSTATUS status, __out HRESULT & hResult) noexcept;

            // Summary:
            // This method converts an NTSTATUS into HRESULT.
            // Unlike ToSFHResult which converts SF STATUS into SF HRESULT without loss,
            // this method falls back to conversion dictionary for well known error codes.
            // If input status is not SF STATUS and not in the well known dictionary, wrapping is used.
            // 
            static HRESULT ToHResult(__in NTSTATUS status) noexcept;

        private:
            static bool MapToNTStatus(__in HRESULT hResult, __out NTSTATUS & status) noexcept;
            static bool MapToHResult(__in NTSTATUS status, __out HRESULT & hResult) noexcept;

        private:
            // System error code is from 0x0 - 0x3E7F. To build the HRESULT, it masks the error code with 0x8007FFFF.
            // So if it is a Win32 error, the first four bytes should be 0x8007. And to get the win32 error code, we
            // get the last four bytes.
            static bool TryConvertWin32ErrorCode(
                __in HRESULT hResult,
                __out NTSTATUS & status) noexcept;

        private:
            static const HRESULT HighMask = 0x80071800;
            static const HRESULT LowMask = 0x000007FF;

            static const HRESULT HighBytesMask = 0xFFFF0000;
            static const HRESULT LowBytesMask = 0x0000FFFF;
            static const HRESULT Win32HighBytesMask = (FACILITY_WIN32 << 16) | 0x80000000;

            // Mask used to zero out everything but the severity bit until the highest reserved error code bit.
            static const LONG SFLowMask = 0x8003FFFF;
            static const LONG SFStatusHighMask = 0x60400000;

            static const LONG LowestSFErrorCode = 0x3BC;
            static const LONG HighestSFErrorCode = 0x54B;

        private:
            static const LONG LowestSFStatus = 0xE0431BBC;
            static const LONG HighestSFStatus = 0xE0431D4B;
            static const LONG SFHResultHighMask = 0x80071800;
            
        };
    }
}
