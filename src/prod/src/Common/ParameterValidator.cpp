// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    const size_t ParameterValidator::MinStringSize = 1;
    const size_t ParameterValidator::MaxStringSize = 4 * 1024;
    const size_t ParameterValidator::MaxConnectionStringSize = 4 * 1024;
    const size_t ParameterValidator::MaxNameSize = 4 * 1024;
    const size_t ParameterValidator::MaxEndpointSize = 4 * 1024 * 1024;
    const size_t ParameterValidator::MaxFilePathSize = 32 * 1024;

    StringLiteral const TraceCategory("ParameterValidator");

    ErrorCode ParameterValidator::IsValid(
        LPCWSTR param,
        size_t minAllowedLength,
        size_t maxAllowedLength)
    {
        size_t length;
        return IsValid(param, minAllowedLength, maxAllowedLength, length);
    }

    ErrorCode ParameterValidator::IsValidTruncated(
        LPCWSTR param,
        size_t minAllowedLength,
        __inout size_t & length)
    {
        // Compare with max allowed string size to get the actual string length
        return IsValid(param, minAllowedLength, STRSAFE_MAX_CCH, length);
    }

    ErrorCode ParameterValidator::IsValid(
        LPCWSTR param, 
        size_t minAllowedLength, 
        size_t maxAllowedLength,
        __inout size_t & length) 
    {
        if (param == NULL)
        {
            return ErrorCodeValue::ArgumentNull;
        }

        HRESULT hr = StringCchLength(param, maxAllowedLength, &length);

        if (!SUCCEEDED(hr))
        {
            return TraceAndCreateError(
                ErrorCodeValue::InvalidArgument, 
                wformatString(GET_COMMON_RC(String_Too_Long2), wstring(param, maxAllowedLength), maxAllowedLength));
        }

        if (length < minAllowedLength)
        {
            return TraceAndCreateError(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(String_Too_Short), wstring(param), minAllowedLength));
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ParameterValidator::ValidatePercentValue(BYTE percentValue, Common::WStringLiteral const & parameterName)
    {
        if ((percentValue < 0) || (percentValue > 100))
        {
            return TraceAndCreateError(
                ErrorCodeValue::InvalidArgument, 
                wformatString("{0} {1}={2}.", GET_COMMON_RC(Invalid_Percent_Value), parameterName, percentValue));
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    ErrorCode ParameterValidator::TraceAndCreateError(ErrorCodeValue::Enum error, wstring && errorMessage)
    {
        Trace.WriteWarning(TraceCategory, "{0}", errorMessage);

        return ErrorCode(error, move(errorMessage));
    }
}
