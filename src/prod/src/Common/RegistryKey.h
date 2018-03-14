// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class RegistryKey
    {
    public:
        explicit RegistryKey(std::wstring const & name, bool const readOnly = true, bool const openExisting = false);

        RegistryKey(std::wstring const & name, LPCWSTR machineName, bool const readOnly = true, bool const openExisting = false);

        bool GetValue(std::wstring const & name, DWORD & value);

        bool GetValue(std::wstring const & name, std::wstring & value);

        bool GetValue(std::wstring const & name, std::wstring & value, bool expandEnvironmentStrings);

        bool GetValue(std::wstring const & name, std::vector<std::wstring> & value);

        bool SetValue(std::wstring const & name, DWORD value);

        bool SetValue(std::wstring const & name, std::wstring const & value, bool typeRegSZ = false);

        bool SetValue(std::wstring const & name, std::vector<std::wstring> const & value);

        bool SetValue(std::wstring const & name, const BYTE * value, ULONG valueLen);

        bool DeleteValue(std::wstring const & name);

        bool DeleteKey();

        ~RegistryKey();

        __declspec( property(get=GetExisted)) bool Existed;
        bool GetExisted() { return existed_; }

        __declspec( property(get=GetIsValid)) bool IsValid;
        bool GetIsValid() { return error_ == ERROR_SUCCESS; }

        __declspec( property(get=GetError)) DWORD Error;
        DWORD GetError() { return error_; }

    private:
        DENY_COPY(RegistryKey);
        
        std::wstring keyName_;
        HKEY key_;
        bool existed_;
        bool initialized_;
        DWORD error_;
    };
}
