// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureSecurityPrincipalRequest : public Serialization::FabricSerializable
    {
        DENY_COPY(ConfigureSecurityPrincipalRequest)

    public:
        ConfigureSecurityPrincipalRequest();
        ConfigureSecurityPrincipalRequest(
            std::wstring const & nodeId,
            std::wstring const & applicationId,
            ULONG applicationPackageCounter, 
            ServiceModel::PrincipalsDescription const & principalsDescription,
            int const allowedUserCreationFailureCount,
            bool const configureForNode,
            bool const updateExisting);

        ConfigureSecurityPrincipalRequest(ConfigureSecurityPrincipalRequest && other);
        ConfigureSecurityPrincipalRequest & operator=(ConfigureSecurityPrincipalRequest && other);

        __declspec(property(get=get_PrincipalsDescription)) ServiceModel::PrincipalsDescription const & Principals;
        ServiceModel::PrincipalsDescription const & get_PrincipalsDescription() const { return principalsDescription_; }

        __declspec(property(get=get_ApplicationId)) std::wstring const & ApplicationId;
        std::wstring const & get_ApplicationId() const { return applicationId_; }

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_AllowedUserCreationFailureCount)) int const AllowedUserCreationFailureCount;
        int const get_AllowedUserCreationFailureCount() const { return allowedUserCreationFailureCount_; }

        __declspec(property(get = get_IsConfiguredForNode)) bool const  IsConfiguredForNode;
        bool const get_IsConfiguredForNode() const { return configureForNode_; }

        __declspec(property(get=get_ApplicationPackageCounter)) ULONG ApplicationPackageCounter;
        ULONG get_ApplicationPackageCounter() const { return applicationPackageCounter_; }

        __declspec(property(get = get_UpdateExisting)) bool const UpdateExisting;
        bool const get_UpdateExisting() const { return updateExisting_; }

        std::wstring && TakeApplicationId() { return std::move(applicationId_); }
        std::wstring && TakeNodeId() { return std::move(nodeId_); }
        ServiceModel::PrincipalsDescription && TakePrincipalsDescription() { return std::move(principalsDescription_); }

        bool HasEmptyGroupsAndUsers() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(nodeId_, applicationId_, applicationPackageCounter_, principalsDescription_, allowedUserCreationFailureCount_, configureForNode_, updateExisting_);

    private:
        std::wstring nodeId_;
        std::wstring applicationId_;
        ULONG applicationPackageCounter_;
        ServiceModel::PrincipalsDescription principalsDescription_;
        int allowedUserCreationFailureCount_;
        bool configureForNode_;
        bool updateExisting_;
    };
}
