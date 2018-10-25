//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(VolumeDescription)
INITIALIZE_SIZE_ESTIMATION(VolumeDescriptionAzureFile)
INITIALIZE_SIZE_ESTIMATION(VolumeDescriptionVolumeDiskBase)

VolumeDescription::VolumeDescription()
    : provider_(VolumeProvider::Invalid)
{
}

Serialization::IFabricSerializable * VolumeDescription::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(VolumeProvider::Enum))
    {
        return nullptr;
    }

    VolumeProvider::Enum provider = *(reinterpret_cast<VolumeProvider::Enum const *>(typeInformation.buffer));
    switch (provider)
    {
    case VolumeProvider::Enum::AzureFile:
        return new VolumeDescriptionAzureFile();
    case VolumeProvider::Enum::ServiceFabricVolumeDisk:
        return new VolumeDescriptionVolumeDisk();
    default:
        return new VolumeDescription();
    }
}

NTSTATUS VolumeDescription::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&provider_);
    typeInformation.length = sizeof(provider_);
    return STATUS_SUCCESS;
}

VolumeDescriptionSPtr VolumeDescription::CreateSPtr(VolumeProvider::Enum provider)
{
    switch (provider)
    {
    case VolumeProvider::Enum::AzureFile:
        return make_shared<VolumeDescriptionAzureFile>();
    case VolumeProvider::Enum::ServiceFabricVolumeDisk:
        return make_shared<VolumeDescriptionVolumeDisk>();
    default:
        return make_shared<VolumeDescription>();
    }
}

void VolumeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("[VolumeName {0}, Provider {1},", volumeName_, provider_);
}

ErrorCode VolumeDescription::Validate() const
{
    if (provider_ == VolumeProvider::Enum::Invalid)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, GET_CM_RC(VolumeKindInvalid));
    }
    if (volumeName_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, GET_CM_RC(VolumeNameNotSpecified));
    }
    return ErrorCode::Success();
}

ErrorCode VolumeDescriptionAzureFile::Validate() const
{
    ErrorCode error = VolumeDescription::Validate();
    if (!error.IsSuccess())
    {
        return error;
    }
    if (accountName_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_CM_RC(AzureFileVolumeAccountNameNotSpecified), volumeName_));
    }
    if (accountKey_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_CM_RC(AzureFileVolumeAccountKeyNotSpecified), volumeName_));
    }
    if (shareName_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_CM_RC(AzureFileVolumeShareNameNotSpecified), volumeName_));
    }
    return error;
}

void VolumeDescriptionAzureFile::WriteTo(TextWriter & w, FormatOptions const & fOpt) const
{
    VolumeDescription::WriteTo(w, fOpt);
    w.Write("AccountName {0}, ShareName {1}]", accountName_, shareName_);
}

void VolumeDescriptionAzureFile::RemoveSensitiveInformation()
{
    this->accountKey_ = GET_CM_RC(AzureFileVolumeAccountKeyNotDisplayed);
}

ErrorCode VolumeDescriptionVolumeDisk::Validate() const
{
    return VolumeDescriptionVolumeDiskBase::Validate(volumeName_, MaxVolumeNameLength);
}
