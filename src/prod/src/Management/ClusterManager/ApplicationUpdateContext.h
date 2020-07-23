// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {

        //
        // *** ApplicationUpgradeContext
        //
        class ApplicationUpdateContext : public RolloutContext
        {
            DENY_COPY(ApplicationUpdateContext)

        public:
            static const RolloutContextType::Enum ContextType;

            ApplicationUpdateContext(
                Common::ComponentRoot const & replica,
                ClientRequestSPtr const & clientRequest,
                Common::NamingUri const & applicationName,
                ServiceModelApplicationId const & applicationId,
                Reliability::ApplicationCapacityDescription const & currentCapacity,
                Reliability::ApplicationCapacityDescription const & updatedCapacity);

            ApplicationUpdateContext(
                ApplicationUpdateContext &&);

            ApplicationUpdateContext & operator=(
                ApplicationUpdateContext &&);

            explicit ApplicationUpdateContext(
                Common::NamingUri const &);

            __declspec(property(get = get_ApplicationName)) Common::NamingUri const& ApplicationName;
            Common::NamingUri const& get_ApplicationName() const { return applicationName_; }

            __declspec(property(get = get_ApplicationId)) ServiceModelApplicationId const& ApplicationId;
            ServiceModelApplicationId const& get_ApplicationId() const { return applicationId_; }

            __declspec(property(get = get_CurrentCapacities)) Reliability::ApplicationCapacityDescription const& CurrentCapacities;
            Reliability::ApplicationCapacityDescription const& get_CurrentCapacities() const { return currentCapacities_; }

            __declspec(property(get = get_UpdatedCapacities)) Reliability::ApplicationCapacityDescription const& UpdatedCapacities;
            Reliability::ApplicationCapacityDescription const& get_UpdatedCapacities() const { return updatedCapacities_; }

            void OnFailRolloutContext() override { }

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(applicationName_, applicationId_, currentCapacities_, updatedCapacities_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            // Used as key.
            Common::NamingUri applicationName_;

            // Application
            ServiceModelApplicationId applicationId_;

            // Old values, as persisted in ApplicationContext until update operation finishes succesfully.
            Reliability::ApplicationCapacityDescription currentCapacities_;

            // New values that will be written to ApplicationContext eventually
            Reliability::ApplicationCapacityDescription updatedCapacities_;
        };
    }
}
