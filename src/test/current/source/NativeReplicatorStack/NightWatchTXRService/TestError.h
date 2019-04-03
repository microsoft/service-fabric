// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    class TestError
        : public Common::IFabricJsonSerializable
    {
    public:

        TestError()
            : msg_(L"")
            , status_(TestStatus::Enum::Invalid)
        {
        }

        TestError(
            __in std::wstring const & msg,
            __in TestStatus::Enum const & status)
            : msg_(msg)
            , status_(status)
        {
        }

        __declspec(property(get = get_msg)) std::wstring Msg;
        std::wstring get_msg() const
        {
            return msg_;
        }

        _declspec(property(get = get_status)) TestStatus::Enum Status;
        TestStatus::Enum get_status() const
        {
            return status_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "Msg = '{0}', Status = '{1}'",
                msg_,
                TestStatus::ToString(status_));
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Msg", msg_)
            SERIALIZABLE_PROPERTY_ENUM(L"Status", status_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring msg_;
        TestStatus::Enum status_;
    };
}
