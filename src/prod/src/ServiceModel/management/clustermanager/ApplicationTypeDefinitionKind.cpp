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
        namespace ApplicationTypeDefinitionKind
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Enum::ServiceFabricApplicationPackage: w << "ServiceFabricApplicationPackage"; return;
                    case Enum::Compose: w << "Compose"; return;
                    case Enum::MeshApplicationDescription: w << "MeshApplicationDescription"; return;
                };

                w << "ApplicationTypeDefinitionKind(" << static_cast<int>(e) << ')';
            }

            bool IsMatch(DWORD const publicFilter, Enum const & value)
            {
                if (publicFilter == FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT || publicFilter == FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_ALL)
                {
                    return true;
                }

                if (publicFilter & FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_SERVICE_FABRIC_APPLICATION_PACKAGE && value == Enum::ServiceFabricApplicationPackage)
                {
                    return true;
                }

                if (publicFilter & FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_COMPOSE && value == Enum::Compose)
                {
                    return true;
                }

                if (publicFilter & FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_MESH_APPLICATION_DESCRIPTION && value == Enum::MeshApplicationDescription)
                {
                    return true;
                }

                return false;
            }

            FABRIC_APPLICATION_TYPE_DEFINITION_KIND ToPublicApi(Enum const & toConvert)
            {
                switch (toConvert)
                {
                    case Enum::ServiceFabricApplicationPackage:
                        return FABRIC_APPLICATION_TYPE_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_PACKAGE;

                    case Enum::Compose:
                        return FABRIC_APPLICATION_TYPE_DEFINITION_KIND_COMPOSE;

                    case Enum::MeshApplicationDescription:
                        return FABRIC_APPLICATION_TYPE_DEFINITION_KIND_MESH_APPLICATION_DESCRIPTION;

                    default:
                        return FABRIC_APPLICATION_TYPE_DEFINITION_KIND_INVALID;
                }
            }

            Enum FromPublicApi(FABRIC_APPLICATION_TYPE_DEFINITION_KIND const toConvert)
            {
                switch (toConvert)
                {
                    case FABRIC_APPLICATION_TYPE_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_PACKAGE:
                        return Enum::ServiceFabricApplicationPackage;

                    case FABRIC_APPLICATION_TYPE_DEFINITION_KIND_COMPOSE:
                        return Enum::Compose;

                    case FABRIC_APPLICATION_TYPE_DEFINITION_KIND_MESH_APPLICATION_DESCRIPTION:
                        return Enum::MeshApplicationDescription;

                    default:
                        return Enum::Invalid;
                }
            }
        }
    }
}
