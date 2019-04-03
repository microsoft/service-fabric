// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeleteSingleInstanceDeploymentDescription : public Serialization::FabricSerializable
    {
    public:
        DeleteSingleInstanceDeploymentDescription() = default;

        explicit DeleteSingleInstanceDeploymentDescription(std::wstring const & deploymentName);

        DeleteSingleInstanceDeploymentDescription(DeleteSingleInstanceDeploymentDescription const & other) = default;

        DeleteSingleInstanceDeploymentDescription operator=(DeleteSingleInstanceDeploymentDescription const & other);

        __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
        std::wstring const & get_DeploymentName() const { return deploymentName_; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext &) const;
        void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(deploymentName_);

    private:
        std::wstring deploymentName_;
    };
}
