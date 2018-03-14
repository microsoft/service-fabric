// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class CodePackageIdentifier;
    typedef std::unique_ptr<CodePackageIdentifier> CodePackageIdentifierUPtr;

    class CodePackageIdentifier : public Serialization::FabricSerializable
    {
    public:
        CodePackageIdentifier();
        CodePackageIdentifier::CodePackageIdentifier(
            ServicePackageIdentifier const& servicePackageId, 
            std::wstring const & codePackageName);
        CodePackageIdentifier(CodePackageIdentifier const & other);
        CodePackageIdentifier(CodePackageIdentifier && other);

        __declspec(property(get=get_CodePackageName)) std::wstring const & CodePackageName;
        inline std::wstring const & get_CodePackageName() const { return codePackageName_; };

        __declspec(property(get=get_ServicePackageId)) ServicePackageIdentifier const & ServicePackageId;
        inline ServicePackageIdentifier const & get_ServicePackageId() const { return servicePackageId_; };

        CodePackageIdentifier const & operator = (CodePackageIdentifier const & other);
        CodePackageIdentifier const & operator = (CodePackageIdentifier && other);

        bool operator == (CodePackageIdentifier const & other) const;
        bool operator != (CodePackageIdentifier const & other) const;

        int compare(CodePackageIdentifier const & other) const;
        bool operator < (CodePackageIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            std::string format = "CodePackageIdentifier {ServicePackageId = {0}, CodePackageName = {1}}";
            size_t index = 0;

            traceEvent.AddEventField<ServiceModel::ServicePackageIdentifier>(format, name + ".servicePackageId", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".codePackageName", index);

            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<ServiceModel::ServicePackageIdentifier>(servicePackageId_);
            context.Write<std::wstring>(codePackageName_);
        }

        static Common::ErrorCode FromEnvironmentMap(Common::EnvironmentMap const & envMap, __out CodePackageIdentifier & codePackageIdentifier);
        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;

        FABRIC_FIELDS_02(servicePackageId_, codePackageName_);
    private:
        ServicePackageIdentifier servicePackageId_;
        std::wstring codePackageName_;

        static Common::GlobalWString EnvVarName_CodePackageName;
    };
}
