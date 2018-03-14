// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ExeHostWorkingFolder
    {
        enum Enum
        {
            Invalid = 0,
            Work = 1,
            CodePackage = 2,
            CodeBase = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        Common::ErrorCode FromPublicApi(FABRIC_EXEHOST_WORKING_FOLDER const & publicVal, __out Enum & val);
        FABRIC_EXEHOST_WORKING_FOLDER ToPublicApi(Enum const & val);
        bool TryParseFromString(std::wstring const& string, __out Enum & val);
        static std::wstring GetValidValues();
		static std::wstring EnumToString(Enum val);
    };

    struct EntryPointDescription;
    struct CodePackageDescription;
    struct ExeEntryPointDescription : public Serialization::FabricSerializable
    {
    public:
        ExeEntryPointDescription();
        ExeEntryPointDescription(ExeEntryPointDescription const & other) = default;
        ExeEntryPointDescription(ExeEntryPointDescription && other) = default;

        ExeEntryPointDescription & operator = (ExeEntryPointDescription const & other) = default;
        ExeEntryPointDescription & operator = (ExeEntryPointDescription && other) = default;

        bool operator == (ExeEntryPointDescription const & other) const;
        bool operator != (ExeEntryPointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_04(Program, Arguments, WorkingFolder, IsExternalExecutable);

    public:
        std::wstring Program;
        std::wstring Arguments;
        ExeHostWorkingFolder::Enum WorkingFolder;
        bool IsExternalExecutable;
        
        ULONG PeriodicIntervalInSeconds;
        bool ConsoleRedirectionEnabled;
        ULONG ConsoleRedirectionFileRetentionCount;
        ULONG ConsoleRedirectionFileMaxSizeInKb;

    private:
        friend struct CodePackageDescription;
        friend struct EntryPointDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        static const ULONG DefaultConsoleRedirectionFileRetentionCount = 2;
        static const ULONG DefaultConsoleRedirectionFileMaxSizeInKb = 20480;
    };
}
