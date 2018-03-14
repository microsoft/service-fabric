// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    //Does not do any synchronization. Locking to be managed by caller
    class CommandLineParser
    {
        DENY_COPY(CommandLineParser);
    public:

        CommandLineParser(Common::StringCollection const& params, size_t startAt = 0);

        bool TryGetInt64(std::wstring const & name, int64 & value, int64 defaultValue = 0) const;
        bool TryGetLargeInteger(std::wstring const & name, Common::LargeInteger  & value, Common::LargeInteger defaultValue = Common::LargeInteger::Zero) const;
        bool TryGetInt(std::wstring const & name, int & value, int defaultValue = 0) const;
        bool TryGetULONG(std::wstring const & name, ULONG & value, ULONG defaultValue = 0) const;
        bool TryGetDouble(std::wstring const & name, double & value, double defaultValue = 0) const;
        bool GetBool(std::wstring const & name) const;
        bool TryGetString(std::wstring const & name, std::wstring & value, std::wstring const& defaultValue = L"") const;
        bool GetVector(std::wstring const & name, Common::StringCollection & value, std::wstring const& defaultValue = L"", bool skipEmptyTokens = true) const;
        bool GetMap(std::wstring const & name, std::map<std::wstring, std::wstring> & value) const;
        bool IsMap(std::wstring const & name) const;
        bool TryGetFabricVersionInstance(std::wstring const & name, Common::FabricVersionInstance & value, Common::FabricVersionInstance defaultvalue = Common::FabricVersionInstance::Default) const;

        static USHORT ParsePort(std::wstring const & string, Common::StringLiteral tag);

        template <typename T>
        bool GetMap(
            std::wstring const & name, 
            std::map<std::wstring, T> & value) const
        {
            auto iterator = params_.find(name);
            if(iterator != params_.end())
            {
                std::wstring data = (*iterator).second;
                Common::StringCollection pairs;
                Common::StringUtility::Split<std::wstring>(data, pairs, L",");
                for (auto iter = pairs.begin(); iter != pairs.end(); iter++)
                {
                    Common::StringCollection pair;
                    std::wstring pairString = (*iter);
                    Common::StringUtility::Split<std::wstring>(pairString, pair, L":");
                    TestSession::FailTestIfNot(pair.size() == 2, "Invalid format for arg {0}", data);
                    TestSession::FailTestIfNot(StringUtility::TryFromWString<T>(pair[1], value[pair[0]]), "Could not parse {0}", pair[1]);
                }

                return true;
            }
            else
            {
                value.clear();
                return false;
            }
        }

    private:
        mutable std::map<std::wstring, std::wstring> params_;

        void Parse(Common::StringCollection const& params, size_t startAt) const;
    };
};
