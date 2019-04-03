// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace TracePoints
{
    class TracePointPayload
    {
    public:
        TracePointPayload();

        void Parse(std::wstring const & message);

        USHORT GetMessageId() const;

        std::wstring GetFullyQualifiedPipeName() const;

        std::wstring GetNodeId() const;

        bool HasAction() const;

        std::wstring GetAction() const;

    private:
        std::wstring message_;

        USHORT messageId_;

        std::wstring fullyQualifiedPipeName_;

        std::wstring nodeId_;

        bool hasAction_;

        std::wstring action_;

        std::vector<std::wstring> split(const std::wstring &s, std::wstring const& delimiters);
    };
}
