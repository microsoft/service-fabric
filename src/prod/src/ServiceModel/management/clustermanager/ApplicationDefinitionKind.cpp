// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationDefinitionKind
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Enum::ServiceFabricApplicationDescription: w << "ServiceFabricApplicationDescription"; return;
                    case Enum::Compose: w << "Compose"; return;
                };

                w << "ApplicationDefinitionKind(" << static_cast<int>(e) << ')';
            }

            bool IsMatch(DWORD const publicFilter, Enum const & value)
            {
                if (publicFilter == FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT || publicFilter == FABRIC_APPLICATION_DEFINITION_KIND_FILTER_ALL)
                {
                    return true;
                }

                if (publicFilter & FABRIC_APPLICATION_DEFINITION_KIND_FILTER_SERVICE_FABRIC_APPLICATION_DESCRIPTION && value == Enum::ServiceFabricApplicationDescription)
                {
                    return true;
                }

                if (publicFilter & FABRIC_APPLICATION_DEFINITION_KIND_FILTER_COMPOSE && value == Enum::Compose)
                {
                    return true;
                }

                return false;
            }

            FABRIC_APPLICATION_DEFINITION_KIND ToPublicApi(Enum const & toConvert)
            {
                switch (toConvert)
                {
                    case Enum::ServiceFabricApplicationDescription:
                        return FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION;

                    case Enum::Compose:
                        return FABRIC_APPLICATION_DEFINITION_KIND_COMPOSE;

                    default:
                        return FABRIC_APPLICATION_DEFINITION_KIND_INVALID;
                }
            }

            Enum FromPublicApi(FABRIC_APPLICATION_DEFINITION_KIND const toConvert)
            {
                switch (toConvert)
                {
                    case FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION:
                        return Enum::ServiceFabricApplicationDescription;

                    case FABRIC_APPLICATION_DEFINITION_KIND_COMPOSE:
                        return Enum::Compose;

                    default:
                        return Enum::Invalid;
                }
            }
        }
    }
}
