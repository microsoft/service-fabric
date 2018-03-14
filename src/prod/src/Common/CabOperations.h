// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CabOperations :
        protected TextTraceComponent < TraceTaskCodes::Common >
    {
        using TextTraceComponent<TraceTaskCodes::Common>::WriteInfo;
        using TextTraceComponent<TraceTaskCodes::Common>::WriteError;
        DENY_COPY(CabOperations)

    public:
        static bool IsCabFile(std::wstring const & cabPath);
        static int ExtractAll(__in std::wstring const & cabPath, __in std::wstring const & extractPath);
        static int ExtractFiltered(__in std::wstring const & cabPath, __in std::wstring const & extractPath, __in std::vector<std::wstring> const & filters, __in bool inclusive = true);
        static int ContainsFile(__in std::wstring const & cabPath, __in std::wstring const & checkedFile, __out bool & found);

    private:
        class CabOperation;
        class CabIsCabOperation;
        class CabExtractAllOperation;
        class CabExtractFilteredOperation;
        class CabContainsFileOperation;
    };
}
