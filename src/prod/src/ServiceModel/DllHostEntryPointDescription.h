// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct DllHostEntryPointDescription;
    struct DllHostHostedDllDescription;
    struct DllHostHostedManagedDllDescription : public Serialization::FabricSerializable
    {
    public:
        DllHostHostedManagedDllDescription();
        DllHostHostedManagedDllDescription(DllHostHostedManagedDllDescription const & other);
        DllHostHostedManagedDllDescription(DllHostHostedManagedDllDescription && other);

        DllHostHostedManagedDllDescription const & operator = (DllHostHostedManagedDllDescription const & other);
        DllHostHostedManagedDllDescription const & operator = (DllHostHostedManagedDllDescription && other);

        bool operator == (DllHostHostedManagedDllDescription const & other) const;
        bool operator != (DllHostHostedManagedDllDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_01(AssemblyName);

    public:
        std::wstring AssemblyName;

    private:
        friend struct DllHostEntryPointDescription;
        friend struct DllHostHostedDllDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };

    struct DllHostEntryPointDescription;
    struct DllHostHostedDllDescription;
    struct DllHostHostedUnmanagedDllDescription : public Serialization::FabricSerializable
    {
    public:
        DllHostHostedUnmanagedDllDescription();
        DllHostHostedUnmanagedDllDescription(DllHostHostedUnmanagedDllDescription const & other);
        DllHostHostedUnmanagedDllDescription(DllHostHostedUnmanagedDllDescription && other);

        DllHostHostedUnmanagedDllDescription const & operator = (DllHostHostedUnmanagedDllDescription const & other);
        DllHostHostedUnmanagedDllDescription const & operator = (DllHostHostedUnmanagedDllDescription && other);

        bool operator == (DllHostHostedUnmanagedDllDescription const & other) const;
        bool operator != (DllHostHostedUnmanagedDllDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_01(DllName);

    public:
        std::wstring DllName;

    private:
        friend struct DllHostEntryPointDescription;
        friend struct DllHostHostedDllDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };

    struct DllHostEntryPointDescription;
    struct DllHostHostedDllDescription : public Serialization::FabricSerializable
    {
    public:
        DllHostHostedDllDescription();
        DllHostHostedDllDescription(DllHostHostedDllDescription const & other);
        DllHostHostedDllDescription(DllHostHostedDllDescription && other);

        DllHostHostedDllDescription const & operator = (DllHostHostedDllDescription const & other);
        DllHostHostedDllDescription const & operator = (DllHostHostedDllDescription && other);

        bool operator == (DllHostHostedDllDescription const & other) const;
        bool operator != (DllHostHostedDllDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_03(Kind, Unmanaged, Managed);

    public:
        DllHostHostedUnmanagedDllDescription Unmanaged;
        DllHostHostedManagedDllDescription Managed;
        DllHostHostedDllKind::Enum Kind;

    private:
        friend struct DllHostEntryPointDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };

    struct EntryPointDescription;
    struct DllHostHostedDllDescription;
    struct DllHostEntryPointDescription : public Serialization::FabricSerializable
    {
    public:
        DllHostEntryPointDescription();
        DllHostEntryPointDescription(DllHostEntryPointDescription const & other);
        DllHostEntryPointDescription(DllHostEntryPointDescription && other);

        DllHostEntryPointDescription const & operator = (DllHostEntryPointDescription const & other);
        DllHostEntryPointDescription const & operator = (DllHostEntryPointDescription && other);

        bool operator == (DllHostEntryPointDescription const & other) const;
        bool operator != (DllHostEntryPointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_02(IsolationPolicyType, HostedDlls);

    private:
        friend struct EntryPointDescription;
        friend struct DllHostHostedDllDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        CodePackageIsolationPolicyType::Enum ParseIsolationPolicyType(Common::XmlReaderUPtr const & xmlReader, std::wstring const & value);
		std::wstring EnumToString();

    public:
        CodePackageIsolationPolicyType::Enum IsolationPolicyType;
        std::vector<DllHostHostedDllDescription> HostedDlls;
    };
}
    
DEFINE_USER_ARRAY_UTILITY(ServiceModel::DllHostHostedDllDescription);
