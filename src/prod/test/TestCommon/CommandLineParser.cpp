// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
namespace TestCommon
{
    using namespace std;
    using namespace Common;

    bool IsTrue(wstring const & input)
    {
        return StringUtility::AreEqualCaseInsensitive(input, L"true") ||
            StringUtility::AreEqualCaseInsensitive(input, L"yes") ||
            StringUtility::AreEqualCaseInsensitive(input, L"t") ||
            StringUtility::AreEqualCaseInsensitive(input, L"y");
    }

    bool IsFalse(wstring const & input)
    {
        return StringUtility::AreEqualCaseInsensitive(input, L"false") ||
            StringUtility::AreEqualCaseInsensitive(input, L"no") ||
            StringUtility::AreEqualCaseInsensitive(input, L"f") ||
            StringUtility::AreEqualCaseInsensitive(input, L"n");
    }

    CommandLineParser::CommandLineParser(StringCollection const& params, size_t startAt)
        : params_()
    {
        Parse(params, startAt);
    }

    USHORT CommandLineParser::ParsePort(std::wstring const & string, Common::StringLiteral tag)
    {
        int value = Common::Int32_Parse(string);
        TestSession::FailTestIf(value < 0, "Port range {0} must be greater than zero {1}", tag, value);
        TestSession::FailTestIf(value >= std::numeric_limits<USHORT>::max(), "Port range {0} {1} must be less than {2}", tag, value, std::numeric_limits<USHORT>::max());
        return static_cast<USHORT>(value);
    }

    void CommandLineParser::Parse(StringCollection const& params, size_t startAt) const
    {
        for(size_t i = startAt; i < params.size(); i++)
        {
            wstring param = params[i];
            size_t index = 0;
            wstring dash(L"-");
            wstring slash(L"/");
            if(StringUtility::StartsWith(param, dash) || StringUtility::StartsWith(param, slash))
            {
                param = param.substr(1, param.size() - 1);
                index = param.find_first_of(L":");
            }
            else
            {
                index = param.find_first_of(L"=");
            }

            if (index != wstring::npos)
            {
                wstring name = param.substr(0, index);
                wstring value = param.substr(index + 1, param.size());
                params_[name] = value;
            }
            else
            {
                wstring name = param.substr(0, param.size());
                params_[name] = L"";
            }
        }
    }

    bool CommandLineParser::TryGetInt64(wstring const & name, int64 & value, int64 defaultValue) const
    {
        auto iterator = params_.find(name);
        if (iterator != params_.end() && Common::TryParseInt64(iterator->second, value))
        {
            return true;
        }
        else
        {
            value = defaultValue;
            return false;
        }
    }

    bool CommandLineParser::TryGetLargeInteger(wstring const & name, LargeInteger  & value, LargeInteger defaultValue) const
    {           
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            if (LargeInteger::TryParse((*iterator).second, value))
            {
                return true;
            }
        }
        
        value = defaultValue;
        return false;        
    }

    bool CommandLineParser::TryGetInt(wstring const & name, int & value, int defaultValue) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            value = Int32_Parse((*iterator).second);
            return true;
        }
        else
        {
            value = defaultValue;
            return false;
        }
    }

    bool CommandLineParser::TryGetDouble(wstring const & name, double & value, double defaultValue) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            value = Double_Parse((*iterator).second);
            return true;
        }
        else
        {
            value = defaultValue;
            return false;
        }
    }

    bool CommandLineParser::TryGetULONG(wstring const & name, ULONG & value, ULONG defaultValue) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            TestSession::FailTestIfNot(StringUtility::TryFromWString((*iterator).second, value), "Could not parse as ULONG {0}", (*iterator).second);
            return true;
        }
        else
        {
            value = defaultValue;
            return false;
        }
    }


    bool CommandLineParser::GetBool(wstring const & name) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            wstring value = (*iterator).second;
            if(value.empty())
            {
                return true;
            }
            else if (!IsTrue(value) && !IsFalse(value))
            {
                //Invalid text so returning false
                return false;
            }

            return IsTrue(value);
        }
        else
        {
            return false;
        }
    }

    bool CommandLineParser::TryGetString(wstring const & name, wstring & value, wstring const& defaultValue) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            value = (*iterator).second;
            return true;
        }
        else
        {
            value = defaultValue;
            return false;
        }
    }

    bool CommandLineParser::GetVector(wstring const & name, StringCollection & value, wstring const& defaultValue, bool skipEmptyTokens) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            wstring data = (*iterator).second;
            StringUtility::Split<wstring>(data, value, L",", skipEmptyTokens);
            return true;
        }
        else if (!defaultValue.empty())
        {
            StringUtility::Split<wstring>(defaultValue, value, L",", skipEmptyTokens);
            return true;
        }
        else
        {
            value.clear();
            return false;
        }
    }

    bool CommandLineParser::IsMap(wstring const & name) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            return StringUtility::Contains<wstring>(iterator->second, L":");
        }
        else
        {
            return false;
        }
    }

    bool CommandLineParser::GetMap(wstring const & name, map<wstring, wstring> & value) const
    {
        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            wstring data = (*iterator).second;
            StringCollection pairs;
            StringUtility::Split<wstring>(data, pairs, L",");
            for (auto iter = pairs.begin(); iter != pairs.end(); iter++)
            {
                StringCollection pair;
                wstring pairString = (*iter);
                StringUtility::Split<wstring>(pairString, pair, L":");
                TestSession::FailTestIfNot(pair.size() == 2, "Invalid format for arg {0}", data);
                value[pair[0]] = pair[1];
            }

            return true;
        }
        else
        {
            value.clear();
            return false;
        }
    }

    bool CommandLineParser::TryGetFabricVersionInstance(wstring const & name, FabricVersionInstance & value, FabricVersionInstance defaultvalue) const
    {
        FabricVersion tempFabVer;

        auto iterator = params_.find(name);
        if(iterator != params_.end())
        {
            wstring data = (*iterator).second;
            if (FabricVersion::TryParse(data, tempFabVer))
            {
                data += *FabricVersionInstance::Delimiter;
                data += L"0";
            }

            if (FabricVersionInstance::FromString(data, value).IsSuccess())
            {
                return true;
            }                
        }

        value = defaultvalue;
        return false;        
    }
}
