// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    // Created at the HostingSubsystem and saved to Environment for Activated hosts. Read back by the ApplicationHost 
    // from the Environment for Activated hosts. For Non-Activated hosts read from configuration file
    class CodePackageContext : public Serialization::FabricSerializable
    {
    public:
        CodePackageContext();
        CodePackageContext(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            int64 codePackageInstanceSeqNum,
            int64 servicePackageInstanceSeqNum,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::wstring const & applicationName);

        CodePackageContext(CodePackageContext const & other);
        CodePackageContext(CodePackageContext && other);

        __declspec(property(get=get_CodePackageInsatnceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        CodePackageInstanceIdentifier const & get_CodePackageInsatnceId() const { return codePackageInstanceId_; };

        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; };

        __declspec(property(get=get_CodePackageInstanceSeqNum)) int64 const CodePackageInstanceSeqNum;
        int64 const get_CodePackageInstanceSeqNum() const { return codePackageInstanceSeqNum_; };

        __declspec(property(get=get_ServicePackageInstanceSeqNum)) int64 const ServicePackageInstanceSeqNum;
        int64 const get_ServicePackageInstanceSeqNum() const { return servicePackageInstanceSeqNum_; };

        __declspec(property(get=get_ServicePackageVersionInstance)) ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance;
        ServiceModel::ServicePackageVersionInstance const & get_ServicePackageVersionInstance() const { return servicePackageVersionInstance_; };

        CodePackageContext const & operator = (CodePackageContext const & other);
        CodePackageContext const & operator = (CodePackageContext && other);

        bool operator == (CodePackageContext const & other) const;
        bool operator != (CodePackageContext const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;

        static Common::ErrorCode FromEnvironmentMap(Common::EnvironmentMap const & envMap, __out CodePackageContext & codePackageContext);

        FABRIC_FIELDS_05(
            codePackageInstanceId_, 
            codePackageInstanceSeqNum_, 
            servicePackageInstanceSeqNum_, 
            servicePackageVersionInstance_, 
            applicationName_);

    private:
        CodePackageInstanceIdentifier codePackageInstanceId_;
        std::wstring applicationName_;
        int64 codePackageInstanceSeqNum_;
        int64 servicePackageInstanceSeqNum_;
        ServiceModel::ServicePackageVersionInstance servicePackageVersionInstance_;

        static Common::GlobalWString EnvVarName_ServicePackageInstanceSeqNum;
        static Common::GlobalWString EnvVarName_CodePackageInstanceSeqNum;
        static Common::GlobalWString EnvVarName_ApplicationName;
    };
}
