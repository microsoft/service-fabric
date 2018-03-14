// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace ComposeDeploymentStatus
    {
        std::wstring ToString(Enum const & val)
        {
            wstring composeApplicationStatus;
            StringWriter(composeApplicationStatus).Write(val);
            return composeApplicationStatus;
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Invalid: w << "Invalid"; return;
                case Provisioning: w << "Provisioning"; return;
                case Creating: w << "Creating"; return;
                case Ready: w << "Ready"; return;
                case Deleting: w << "Deleting"; return;
                case Unprovisioning: w << "Unprovisioning"; return;
                case Failed: w << "Failed"; return;
                case Upgrading: w << "Upgrading"; return;
                default: w << "ComposeDeploymentStatus(" << static_cast<int>(e) << ')'; return;
            }
        }

        FABRIC_COMPOSE_DEPLOYMENT_STATUS ToPublicApi(Enum const & status)
        {
            switch (status)
            {
                case ComposeDeploymentStatus::Provisioning:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_PROVISIONING;

                case ComposeDeploymentStatus::Creating:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_CREATING;

                case ComposeDeploymentStatus::Ready:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_READY;

                case ComposeDeploymentStatus::Deleting:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_DELETING;

                case ComposeDeploymentStatus::Unprovisioning:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_UNPROVISIONING;

                case ComposeDeploymentStatus::Failed:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_FAILED;

                case ComposeDeploymentStatus::Upgrading:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_UPGRADING;

                default:
                    return FABRIC_COMPOSE_DEPLOYMENT_STATUS_INVALID;
            }
        }

        Enum FromPublicApi(__in FABRIC_COMPOSE_DEPLOYMENT_STATUS const & publicComposeStatus)
        {
            switch(publicComposeStatus)
            {
                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_PROVISIONING:
                    return Enum::Provisioning;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_CREATING:
                    return Enum::Creating;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_READY:
                    return Enum::Ready;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_DELETING:
                    return Enum::Deleting;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_UNPROVISIONING:
                    return Enum::Unprovisioning;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_FAILED:
                    return Enum::Failed;

                case FABRIC_COMPOSE_DEPLOYMENT_STATUS_UPGRADING:
                    return Enum::Upgrading;

                default:
                    return Enum::Invalid;
            }
        }
    }
}
