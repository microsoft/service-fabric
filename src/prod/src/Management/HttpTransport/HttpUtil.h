// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/KtlSF.Common.h"

namespace HttpCommon
{
    // Common HTTP status codes.  SUCCESS == Ok == 200
    //
    enum HttpStatusCode : unsigned short
    {
        NoResponse = 0,                // Not a valid HTTP code, but used internally

        Upgrade = 101,
        Ok = 200,
        Found = 302,
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        Conflict = 409,
        Gone = 410,
        ContentLengthRequired = 411,
        RequestEntityTooLarge = 413,
        UriTooLong = 414,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        InsufficientResources = 507
    };

    class HttpUtil
    {
    public:

#if !defined(PLATFORM_UNIX)
        static bool HeaderStringToKnownHeaderCode(std::wstring const&, __out KHttpUtils::HttpHeaderType &headerCode);

        static bool VerbToKHttpOpType(std::wstring const &, __out KHttpUtils::OperationType &opType);

        static KAllocator& GetKtlAllocator()
        {
            return Common::GetSFDefaultPagedKAllocator();
        }

        //
        // Given a WinHTTP error code, convert it to an appropriate string
        //
        static std::wstring WinHttpCodeToString(ULONG winhttpError);
#endif
        static bool IsHttpUri(std::wstring const &uri);

        static bool IsHttpsUri(std::wstring const &uri);

        //
        // This method performs a simple translation of ErrorCodeValue to a Http Status code & a short description of the status.
        //
        static void HttpUtil::ErrorCodeToHttpStatus(
            __in Common::ErrorCode error,
            __out USHORT &httpStatus,
            __out wstring &httpStatusLine);
    };
}
