// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

template <> struct FabricSerializableTraits<RvdLogId>
{
    static const bool IsSerializable = true;

    static NTSTATUS FabricSerialize(FabricSerializationHelper * helper, RvdLogId & t)
    {
        return helper->Write(static_cast<GUID&>(t.GetReference()));
    }

    static NTSTATUS FabricDeserialize(FabricSerializationHelper * helper, RvdLogId & t)
    {
        GUID& temp = static_cast<GUID&>(t.GetReference());
        NTSTATUS status = helper->Read(temp);
        CheckReadFieldSuccess(status);
        return status;
    }
};

namespace KtlLogger
{
    class SharedLogSettings : public Serialization::FabricSerializable
    {
        DENY_COPY(SharedLogSettings)

    private:

        template <int size>
        class WCharArrayWrapper
        {
        public:

            WCharArrayWrapper(WCHAR path[size])
                : path_(path)
            {
            }

            NTSTATUS FabricSerialize(FabricSerializationHelper * helper) const
            {
                return helper->WriteByteArray(static_cast<PVOID>(path_), size * sizeof(WCHAR));
            }

            NTSTATUS FabricDeserialize(FabricSerializationHelper * helper) const
            {
                NTSTATUS status = helper->ReadByteArray(static_cast<PVOID>(path_), size * sizeof(WCHAR));
                CheckReadFieldSuccess(status);
                return status;
            }

            PWCHAR path_;
        };

        __declspec(property(get = get_PathWrapper)) KtlLogger::SharedLogSettings::WCharArrayWrapper<512> PathWrapper;
        KtlLogger::SharedLogSettings::WCharArrayWrapper<512> const get_PathWrapper() const { return KtlLogger::SharedLogSettings::WCharArrayWrapper<512>(settings_->Path); }

    public:

        SharedLogSettings()
            : settings_(std::move(make_unique<KtlLogManager::SharedLogContainerSettings>()))
        {
        }

        SharedLogSettings(std::unique_ptr<KtlLogManager::SharedLogContainerSettings> settings)
            : settings_(std::move(settings))
        {
        }

        __declspec(property(get = get_Settings)) KtlLogManager::SharedLogContainerSettings const & Settings;
        KtlLogManager::SharedLogContainerSettings const & get_Settings() const { return *settings_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("SharedLogSettings= { ");
            w.Write("Path = {0} ", settings_->Path);
            w.Write("DiskId = {0} ", Common::Guid(settings_->DiskId));
            w.Write("LogContainerId = {0} ", Common::Guid(static_cast<GUID&>(settings_->LogContainerId.GetReference())));
            w.Write("LogSize = {0} ", settings_->LogSize);
            w.Write("MaximumNumberStream = {0} ", settings_->MaximumNumberStreams);
            w.Write("MaximumRecordSize = {0} ", settings_->MaximumRecordSize);
            w.Write("Flags = {0} ", settings_->Flags);
            w.Write("}");
        }

        FABRIC_FIELDS_07(
            this->PathWrapper,
            settings_->DiskId,
            settings_->LogContainerId,
            settings_->LogSize,
            settings_->MaximumNumberStreams,
            settings_->MaximumRecordSize,
            settings_->Flags);

    private:

        std::unique_ptr<KtlLogManager::SharedLogContainerSettings> settings_;
    };
}
