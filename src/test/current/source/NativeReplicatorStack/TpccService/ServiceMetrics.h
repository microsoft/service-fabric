// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class ServiceMetrics : public Common::IFabricJsonSerializable
    {
    public:

        ServiceMetrics()
            : memoryUsage_(0)
        {
        }

        PROPERTY(LONG64, memoryUsage_, MemoryUsage)

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "MemoryUsage = '{0}'",
                memoryUsage_);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"MemoryUsage", memoryUsage_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        LONG64 memoryUsage_;
    };
}
