// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class ClusterRepairScopeIdentifier 
            : public RepairScopeIdentifierBase
        {
        public:
            ClusterRepairScopeIdentifier();
            ClusterRepairScopeIdentifier(ClusterRepairScopeIdentifier const &);
            ClusterRepairScopeIdentifier(ClusterRepairScopeIdentifier &&);

            bool IsValid() const override { return true; }
            bool IsVisibleTo(RepairScopeIdentifierBase const & scopeBoundary) const override;

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER const &) override;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;
        };

        DEFINE_REPAIR_SCOPE_IDENTIFIER_ACTIVATOR(ClusterRepairScopeIdentifier, FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER)
    }
}
