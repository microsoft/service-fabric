// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace DiagnosticsSinkKind
        {
            std::wstring ToString(Enum const & val)
            {
                wstring sinkKind;
                StringWriter(sinkKind).Write(val);
                return sinkKind;
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Invalid: w << "Invalid"; return;
                    case AzureInternalMonitoringPipeline: w << "AzureInternalMonitoringPipeline"; return;
                    default: w << "DiagnosticsSinkKind(" << static_cast<int>(e) << ')'; return;
                }
            }

            FABRIC_DIAGNOSTICS_SINKS_KIND ToPublicApi(Enum const & status)
            {
                switch (status)
                {
                    case DiagnosticsSinkKind::AzureInternalMonitoringPipeline:
                        return FABRIC_DIAGNOSTICS_SINKS_KIND_AZUREINTERNAL;

                    default:
                        return FABRIC_DIAGNOSTICS_SINKS_KIND_INVALID;
                }
            }

            Enum FromPublicApi(__in FABRIC_COMPOSE_DEPLOYMENT_STATUS const & publicComposeStatus)
            {
                switch(publicComposeStatus)
                {
                    case FABRIC_DIAGNOSTICS_SINKS_KIND_AZUREINTERNAL:
                        return Enum::AzureInternalMonitoringPipeline;

                    default:
                        return Enum::Invalid;
                }
            }
        }
    }
}
