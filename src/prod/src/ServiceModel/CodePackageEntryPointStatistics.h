// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceModel
{
    class CodePackageEntryPointStatistics 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        CodePackageEntryPointStatistics();

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS & publicCodePackageEntryPointStatistics) const; 

        
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS const & publicCodePackageEntryPointStatistics);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::LastExitCode, LastExitCode)
            SERIALIZABLE_PROPERTY(Constants::LastActivationTime, LastExitTime)
            SERIALIZABLE_PROPERTY(Constants::LastExitTime, LastActivationTime)
            SERIALIZABLE_PROPERTY(Constants::LastSuccessfulActivationTime, LastSuccessfulActivationTime)
            SERIALIZABLE_PROPERTY(Constants::LastSuccessfulExitTime, LastSuccessfulExitTime)
            SERIALIZABLE_PROPERTY(Constants::ActivationFailureCount, ActivationFailureCount)
            SERIALIZABLE_PROPERTY(Constants::ContinuousActivationFailureCount, ContinuousActivationFailureCount)
            SERIALIZABLE_PROPERTY(Constants::ExitFailureCount, ExitFailureCount)
            SERIALIZABLE_PROPERTY(Constants::ContinuousExitFailureCount, ContinuousExitFailureCount)
            SERIALIZABLE_PROPERTY(Constants::ActivationCount, ActivationCount)
            SERIALIZABLE_PROPERTY(Constants::ExitCount, ExitCount)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_11(LastExitCode, LastActivationTime, LastExitTime, LastSuccessfulActivationTime, LastSuccessfulExitTime, ActivationFailureCount, ContinuousActivationFailureCount, ExitFailureCount, ContinuousExitFailureCount, ActivationCount, ExitCount)

    public:
        DWORD LastExitCode;
        Common::DateTime LastActivationTime;
        Common::DateTime LastExitTime;
        Common::DateTime LastSuccessfulActivationTime;
        Common::DateTime LastSuccessfulExitTime;
        ULONG ActivationFailureCount;
        ULONG ContinuousActivationFailureCount;
        ULONG ExitFailureCount;
        ULONG ContinuousExitFailureCount;
        ULONG ActivationCount;
        ULONG ExitCount;

    };
}
