// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace DiagnosticsSinkKind
        {
            enum Enum
            {
                Invalid = 0,
                AzureInternalMonitoringPipeline = 1,
            };

            FABRIC_DIAGNOSTICS_SINKS_KIND ToPublicApi(Enum const &);

            Enum FromPublicApi(__in FABRIC_DIAGNOSTICS_SINKS_KIND const & publicKind);

            std::wstring ToString(Enum const & val);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(AzureInternalMonitoringPipeline)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}
