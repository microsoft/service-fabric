// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    class TestParametersBase
        : public Common::IFabricJsonSerializable
    {
    public:

        TestParametersBase()
            : action_(TestAction::Enum::Invalid)
        {
        }

        TestParametersBase(
            __in TestAction::Enum const & action)
            : action_(action)
        {
        }

        _declspec(property(get = get_action)) TestAction::Enum Action;
        TestAction::Enum get_action() const
        {
            return action_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "Action='{0}'",
                TestAction::ToString(action_));
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(L"Action", action_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        TestAction::Enum action_;
    };
}