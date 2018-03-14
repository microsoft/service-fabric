// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeStateName
            {
                void WriteToTextWriter(TextWriter & w, UpgradeStateName::Enum const & e)
                {
                    switch (e)
                    {
                    case None:
                        w << "None";
                        break;

                    case Completed:
                        w << "Completed";
                        break;

                    case FabricUpgrade_RollbackPreviousUpgrade:
                        w << "FabricUpgrade_RollbackPreviousUpgrade";
                        break;

                    case FabricUpgrade_Download:
                        w << "FabricUpgrade_Download";
                        break;

                    case FabricUpgrade_Validate:
                        w << "FabricUpgrade_Validate";
                        break;

                    case FabricUpgrade_ValidateFailed:
                        w << "FabricUpgrade_ValidateFailed";
                        break;

                    case FabricUpgrade_CloseReplicas:
                        w << "FabricUpgrade_CloseReplicas";
                        break;

                    case FabricUpgrade_DownloadFailed:
                        w << "FabricUpgrade_DownloadFailed";
                        break;

                    case FabricUpgrade_UpdateEntitiesOnCodeUpgrade:
                        w << "FabricUpgrade_UpdateEntitiesOnCodeUpgrade";
                        break;

                    case FabricUpgrade_Upgrade:
                        w << "FabricUpgrade_Upgrade";
                        break;

                    case FabricUpgrade_UpgradeFailed:
                        w << "FabricUpgrade_UpgradeFailed";
                        break;

                    case ApplicationUpgrade_EnumerateFTs:
                        w << "ApplicationUpgrade_EnumerateFTs";
                        break;

                    case ApplicationUpgrade_Download:
                        w << "ApplicationUpgrade_Download";
                        break;

                    case ApplicationUpgrade_DownloadFailed:
                        w << "ApplicationUpgrade_DownloadFailed";
                        break;

                    case ApplicationUpgrade_Analyze:
                        w << "ApplicationUpgrade_Analyze";
                        break;

                    case ApplicationUpgrade_AnalyzeFailed:
                        w << "ApplicationUpgrade_AnalyzeFailed";
                        break;

                    case ApplicationUpgrade_Upgrade:
                        w << "ApplicaitonUpgrade_Upgrade";
                        break;

                    case ApplicationUpgrade_UpgradeFailed:
                        w << "ApplicationUpgrade_upgradeFailed";
                        break;

                    case ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded:
                        w << "ApplicationUpgrade_UpdateVersionsAndCloseIfNeeded";
                        break;

                    case ApplicationUpgrade_WaitForCloseCompletion:
                        w << "ApplicationUpgrade_WaitForCloseCompletion";
                        break;

                    case ApplicationUpgrade_WaitForDropCompletion:
                        w << "ApplicationUpgrade_WaitForDropCompletion";
                        break;

                    case ApplicationUpgrade_WaitForReplicaDownCompletionCheck:
                        w << "ApplicationUpgrade_WaitForReplicaDownCompletionCheck";
                        break;

                    case ApplicationUpgrade_ReplicaDownCompletionCheck:
                        w << "ApplicationUpgrade_ReplicaDownCompletionCheck";
                        break;

                    case ApplicationUpgrade_Finalize:
                        w << "ApplicationUpgrade_Finalize";
                        break;

                    case Test1:
                        w << "Test1";
                        break;

                    case Test2:
                        w << "Test2";
                        break;

                    case Invalid:
                        w << "Invalid";
                        break;

                    default:
                        Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
                    };
                }

                ENUM_STRUCTURED_TRACE(UpgradeStateName, None, LastValidEnum);
            }
        }
    }
}

