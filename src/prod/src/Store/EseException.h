// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseException : public std::exception
    {
    public:
        EseException(JET_ERR eseError);

        __declspec (property(get=get_EseError)) JET_ERR EseError;
        JET_ERR get_EseError() const { return eseError_; }

        static bool ExpectSuccess(JET_ERR e)
        {
            return (e == JET_errSuccess);
        }

        static bool ExpectSuccessOrTruncate(JET_ERR e)
        {
            return (e == JET_errSuccess)
                || (e == JET_wrnBufferTruncated);
        }

        static bool ExpectSuccessOrAlreadyInitialized(JET_ERR e)
        {
            return (e == JET_errSuccess)
                || (e == JET_errAlreadyInitialized);
        }

        static bool ExpectSuccessOrNotFound(JET_ERR e)
        {
            return (e == JET_errSuccess)
                || (e == JET_wrnUniqueKey)
                || (e == JET_wrnSeekNotEqual)
                || (e == JET_errRecordNotFound);
        }

        static bool ExpectSuccessOrNoRecord(JET_ERR e)
        {
            return (e == JET_errSuccess)
                || (e == JET_errNoCurrentRecord);
        }

        typedef bool (*ErrorCheck)(JET_ERR);

        static JET_ERR CheckAndReturn(char const * expression, JET_ERR result, ErrorCheck check, std::wstring const & tag)
        {
            if (!check(result))
            {
                EseLocalStore::WriteInfo("check", "ESE call: '{0}' returned '{1}': {2}", expression, result, tag);
            }
            return result;
        }

    private:
        JET_ERR eseError_;
    };
}
#define CALL_ESE_CHECK_NOTHROW_TAG( isAcceptableError, expression, tag ) \
    Store::EseException::CheckAndReturn(#expression, (expression), isAcceptableError, tag);

#define CALL_ESE_CHECK_NOTHROW( isAcceptableError, expression ) \
    CALL_ESE_CHECK_NOTHROW_TAG( isAcceptableError, expression, L"")

#define CALL_ESE_NOTHROW_TAG( expression, tag ) \
    CALL_ESE_CHECK_NOTHROW_TAG(&Store::EseException::ExpectSuccess, (expression), tag)

#define CALL_ESE_NOTHROW( expression ) \
    CALL_ESE_NOTHROW_TAG((expression), L"")
