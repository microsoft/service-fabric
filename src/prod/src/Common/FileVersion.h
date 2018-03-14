// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once // intellisense workaround

namespace Common
{
    class FileVersion :
        Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:

        static std::wstring GetFileVersion(std::wstring const & fileName);
        static unsigned __int64 GetFileVersionNumber(std::wstring const & fileName, bool traceOnError = true);
        static std::wstring GetCurrentExeVersion();
        static unsigned __int64 GetCurrentExeVersionNumber();     
        static std::wstring NumberVersionToString(unsigned __int64 numVer);
    };
};

