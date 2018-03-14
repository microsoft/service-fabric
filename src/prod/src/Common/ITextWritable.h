// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TextWriter;
    class FormatOptions;

    class ITextWritable
    {
    public:
        virtual void WriteTo(TextWriter& w, FormatOptions const &) const = 0;
    };
}
