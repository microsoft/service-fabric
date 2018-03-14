// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestDispatcher
    {
    public:

        virtual bool Open() = 0;

        virtual void Close() {}

        virtual void Reset() {}

        virtual bool ShouldSkipCommand(std::wstring const &) { return false; }

        virtual bool ExecuteCommand(std::wstring command) = 0;

        virtual std::wstring GetState(std::wstring const & param) = 0;

        static Common::StringCollection CompactParameters(Common::StringCollection const & params);

    protected:
        TestDispatcher(){};
        
        Common::ErrorCodeValue::Enum ParseErrorCode(std::wstring const &);
        
        bool TryParseErrorCode(std::wstring const &, __out Common::ErrorCodeValue::Enum &);

        static std::map<std::wstring, Common::ErrorCodeValue::Enum> testErrorCodeMap;
        Common::ExclusiveLock mapLock_;
    };
};
