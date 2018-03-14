// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct ParameterValidator
    {
    public:
        // String Parameter Validation
        static const size_t MinStringSize;
        static const size_t MaxStringSize;
        static const size_t MaxConnectionStringSize;
        static const size_t MaxNameSize;
        static const size_t MaxEndpointSize;
        static const size_t MaxFilePathSize;
        
        // returns error if the string is null or it is not within the specified bounds
        static ErrorCode IsValid(
            LPCWSTR param, 
            size_t minAllowedLength = MinStringSize, 
            size_t maxAllowedLength = MaxStringSize);

        // returns error if the string is null or if it can't be truncated within the specified bounds
        static ErrorCode IsValidTruncated(
            LPCWSTR param,
            size_t minAllowedLength,
            __inout size_t & length);

        static ErrorCode ValidatePercentValue(BYTE percentValue, Common::WStringLiteral const & parameterName);

    private:
        static ErrorCode IsValid(
            LPCWSTR param,
            size_t minAllowedLength,
            size_t maxAllowedLength,
            __inout size_t & length);

        static ErrorCode TraceAndCreateError(ErrorCodeValue::Enum, std::wstring && errorMessage);
    };
}

