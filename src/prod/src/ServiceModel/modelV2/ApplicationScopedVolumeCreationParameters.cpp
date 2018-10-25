//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(ApplicationScopedVolumeCreationParameters)
INITIALIZE_SIZE_ESTIMATION(ApplicationScopedVolumeCreationParametersVolumeDiskBase)

bool ApplicationScopedVolumeCreationParameters::operator==(ApplicationScopedVolumeCreationParameters const & other) const
{
    if (!FabricSerializable::operator==(other))
    {
        return false;
    }

    if (provider_ == other.provider_
        && description_ == other.description_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ApplicationScopedVolumeCreationParameters::operator!=(ApplicationScopedVolumeCreationParameters const & other) const
{
    return !(*this == other);
}

Serialization::IFabricSerializable * ApplicationScopedVolumeCreationParameters::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(ApplicationScopedVolumeProvider::Enum))
    {
        return nullptr;
    }

    ApplicationScopedVolumeProvider::Enum provider = *(reinterpret_cast<ApplicationScopedVolumeProvider::Enum const *>(typeInformation.buffer));
    switch (provider)
    {
    case ApplicationScopedVolumeProvider::Enum::ServiceFabricVolumeDisk:
        return new ApplicationScopedVolumeCreationParametersVolumeDisk();
    default:
        return new ApplicationScopedVolumeCreationParameters();
    }
}

NTSTATUS ApplicationScopedVolumeCreationParameters::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&provider_);
    typeInformation.length = sizeof(provider_);
    return STATUS_SUCCESS;
}

ApplicationScopedVolumeCreationParametersSPtr ApplicationScopedVolumeCreationParameters::CreateSPtr(ApplicationScopedVolumeProvider::Enum provider)
{
    switch (provider)
    {
    case ApplicationScopedVolumeProvider::Enum::ServiceFabricVolumeDisk:
        return make_shared<ApplicationScopedVolumeCreationParametersVolumeDisk>();
    default:
        return make_shared<ApplicationScopedVolumeCreationParameters>();
    }
}

void ApplicationScopedVolumeCreationParameters::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("{Provider {0},", provider_);
}

ErrorCode ApplicationScopedVolumeCreationParameters::Validate() const
{
    if (provider_ == ApplicationScopedVolumeProvider::Enum::Invalid)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, GET_MODELV2_RC(ApplicationScopedVolumeKindInvalid));
    }
    return ErrorCode::Success();
}

ErrorCode ApplicationScopedVolumeCreationParameters::Validate(std::wstring const & volumeName) const
{
    UNREFERENCED_PARAMETER(volumeName);
    return this->Validate();
}

bool ApplicationScopedVolumeCreationParametersVolumeDisk::operator==(ApplicationScopedVolumeCreationParameters const & other) const
{
    if (!ApplicationScopedVolumeCreationParameters::operator==(other))
    {
        return false;
    }

    // The downcasting of "other" below is safe because the above comparison would have already
    // verified that the provider is the same for both objects which means they are of the same type.
    return ApplicationScopedVolumeCreationParametersVolumeDiskBase::operator==(
        static_cast<ApplicationScopedVolumeCreationParametersVolumeDiskBase const &>(other));
}

bool ApplicationScopedVolumeCreationParametersVolumeDisk::operator!=(ApplicationScopedVolumeCreationParameters const & other) const
{
    return !(*this == other);
}

ErrorCode ApplicationScopedVolumeCreationParametersVolumeDisk::Validate(std::wstring const & volumeName) const
{
    return ApplicationScopedVolumeCreationParametersVolumeDiskBase::Validate(volumeName, MaxVolumeNameLength);
}
