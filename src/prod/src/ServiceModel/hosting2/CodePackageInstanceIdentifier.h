// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageInstanceIdentifier;
    typedef std::unique_ptr<CodePackageInstanceIdentifier> CodePackageInstanceIdentifierUPtr;

    class CodePackageInstanceIdentifier : public Serialization::FabricSerializable
    {
    public:
        CodePackageInstanceIdentifier();

        CodePackageInstanceIdentifier::CodePackageInstanceIdentifier(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            std::wstring const & codePackageName);

        CodePackageInstanceIdentifier(CodePackageInstanceIdentifier const & other);
        CodePackageInstanceIdentifier(CodePackageInstanceIdentifier && other);

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        inline std::wstring const & get_CodePackageName() const { return codePackageName_; };

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const { return servicePackageInstanceId_; };

        CodePackageInstanceIdentifier const & operator = (CodePackageInstanceIdentifier const & other);
        CodePackageInstanceIdentifier const & operator = (CodePackageInstanceIdentifier && other);

        bool operator == (CodePackageInstanceIdentifier const & other) const;
        bool operator != (CodePackageInstanceIdentifier const & other) const;

        int compare(CodePackageInstanceIdentifier const & other) const;
        bool operator < (CodePackageInstanceIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & codePackageInstanceIdStr,
            __out CodePackageInstanceIdentifier & codePackageInstanceId);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            std::string format = "CodePackageInstanceIdentifier {ServicePackageInstanceId = {0}, CodePackageName = {1}}";
            size_t index = 0;

            traceEvent.AddEventField<ServicePackageInstanceIdentifier>(format, name + ".servicePackageInstanceId", index);
            traceEvent.AddEventField<std::wstring>(format, name + ".codePackageName", index);

            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<ServicePackageInstanceIdentifier>(servicePackageInstanceId_);
            context.Write<std::wstring>(codePackageName_);
        }

        static Common::ErrorCode FromEnvironmentMap(
            Common::EnvironmentMap const & envMap, 
            __out CodePackageInstanceIdentifier & CodePackageInstanceIdentifier);
        
        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;

        FABRIC_FIELDS_02(servicePackageInstanceId_, codePackageName_);

    private:
        ServicePackageInstanceIdentifier servicePackageInstanceId_;
        std::wstring codePackageName_;

        static Common::GlobalWString Delimiter;
        static Common::GlobalWString EnvVarName_CodePackageName;
    };
}
