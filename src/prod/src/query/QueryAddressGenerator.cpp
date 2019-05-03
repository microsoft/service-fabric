// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Query;
using namespace ServiceModel;

StringLiteral const TraceType("QueryAddressGenerator");

QueryAddressGenerator::QueryAddressGenerator(
    string const & addressString,
    wstring const & arg0,
    wstring const & arg1,
    wstring const & arg2,
    wstring const & arg3,
    wstring const & arg4,
    wstring const & arg5,
    wstring const & arg6,
    wstring const & arg7,
    wstring const & arg8,
    wstring const & arg9)
    : addressString_(addressString)
    , queryArgValues_()
{
    if (arg0 == L"") return;
    queryArgValues_.push_back(arg0);

    if (arg1 == L"") return;
    queryArgValues_.push_back(arg1);

    if (arg2 == L"") return;
    queryArgValues_.push_back(arg2);

    if (arg3 == L"") return;
    queryArgValues_.push_back(arg3);

    if (arg4 == L"") return;
    queryArgValues_.push_back(arg4);

    if (arg5 == L"") return;
    queryArgValues_.push_back(arg5);

    if (arg6 == L"") return;
    queryArgValues_.push_back(arg6);

    if (arg7 == L"") return;
    queryArgValues_.push_back(arg7);

    if (arg8 == L"") return;
    queryArgValues_.push_back(arg8);

    if (arg9 == L"") return;
    queryArgValues_.push_back(arg9);
}

ErrorCode QueryAddressGenerator::GenerateQueryAddress(
    Common::ActivityId const & activityId,
    QueryArgumentMap const & queryArgs,
    _Out_ wstring & address)
{
    auto error = ValidateAddressArguments(activityId, queryArgs);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto argCount = queryArgValues_.size();
    StringLiteral format(addressString_.c_str(), addressString_.c_str() + addressString_.size());

    switch(argCount)
    {
    case 0:
        address = wformatString(
            format);
        break;
    case 1:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]]);
        break;
    case 2:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]]);
        break;
    case 3:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]]);
        break;
    case 4:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]]);
        break;
    case 5:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]],
            queryArgs[queryArgValues_[4]]);
        break;
    case 6:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]],
            queryArgs[queryArgValues_[4]],
            queryArgs[queryArgValues_[5]]);
        break;
    case 7:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]],
            queryArgs[queryArgValues_[4]],
            queryArgs[queryArgValues_[5]],
            queryArgs[queryArgValues_[6]]);
        break;
    case 8:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]],
            queryArgs[queryArgValues_[4]],
            queryArgs[queryArgValues_[5]],
            queryArgs[queryArgValues_[6]],
            queryArgs[queryArgValues_[7]]);
        break;
    case 9:
        address = wformatString(
            format,
            queryArgs[queryArgValues_[0]],
            queryArgs[queryArgValues_[1]],
            queryArgs[queryArgValues_[2]],
            queryArgs[queryArgValues_[3]],
            queryArgs[queryArgValues_[4]],
            queryArgs[queryArgValues_[5]],
            queryArgs[queryArgValues_[6]],
            queryArgs[queryArgValues_[7]],
            queryArgs[queryArgValues_[8]]);
        break;
    default:
        WriteWarning(
                TraceType,
                "Unsupported address format");
        return ErrorCodeValue::InvalidConfiguration;
    }

    return ErrorCode::Success();
}

ErrorCode QueryAddressGenerator::ValidateAddressArguments(
    Common::ActivityId const & activityId,
    QueryArgumentMap const& queryArgs)
{
    for (auto const& arg : queryArgValues_)
    {
        if (!queryArgs.ContainsKey(arg))
        {
            WriteWarning(
                TraceType,
                "{0}: Required query arg {1} not found",
                activityId,
                arg);
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_QUERY_RC(Query_Arg_Not_Found), arg));
        }

        auto pos = queryArgs[arg].find_first_of(*QueryAddress::ReservedCharacters);
        if (pos != wstring::npos)
        {
            WriteWarning(
                TraceType,
                "{0}: Query arg {1} : '{2}' contains an invalid character {3}",
                activityId,
                arg,
                queryArgs[arg],
                queryArgs[arg][pos]);

            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_QUERY_RC(Query_Arg_Invalid_Character), arg, queryArgs[arg], queryArgs[arg][pos]));
        }
    }

    return ErrorCodeValue::Success;
}
