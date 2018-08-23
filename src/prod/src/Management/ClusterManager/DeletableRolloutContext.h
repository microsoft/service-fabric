// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeletableRolloutContext : public RolloutContext
        {
            DENY_COPY_ASSIGNMENT(DeletableRolloutContext)

        public:
            static const RolloutContextType::Enum ContextType;

            explicit DeletableRolloutContext(
                RolloutContextType::Enum);

            DeletableRolloutContext(
                RolloutContextType::Enum,
                bool const);

            DeletableRolloutContext(
                DeletableRolloutContext const &);

            DEFAULT_MOVE_CONSTRUCTOR(DeletableRolloutContext);
            DEFAULT_MOVE_ASSIGNMENT(DeletableRolloutContext);

            DeletableRolloutContext(
                RolloutContextType::Enum,
                Common::ComponentRoot const &,
                ClientRequestSPtr const &);

            DeletableRolloutContext(
                RolloutContextType::Enum,
                Store::ReplicaActivityId const &);

            virtual bool ShouldTerminateProcessing() override;

            __declspec(property(get=get_IsForceDelete, put=put_IsForceDelete)) bool IsForceDelete;
            bool get_IsForceDelete() const { return isForceDelete_; }
            void put_IsForceDelete(bool const value) { isForceDelete_ = value; }

            __declspec(property(get=get_IsConvertToForceDelete, put=put_IsConvertToForceDelete)) bool IsConvertToForceDelete;
            bool get_IsConvertToForceDelete() const override { return isConvertToForceDelete_; }
            void put_IsConvertToForceDelete(bool const value) { isConvertToForceDelete_ = value; }

        protected:
            bool isForceDelete_;
            bool isConvertToForceDelete_;
        };
    }
}
