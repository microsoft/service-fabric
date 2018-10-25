//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::ClusterManager;

StoreDataVolume::StoreDataVolume(
    wstring const & volumeName)
    : volumeName_(volumeName)
    , volumeDescription_()
{
}

StoreDataVolume::StoreDataVolume(
    Management::ClusterManager::VolumeDescriptionSPtr const & volumeDescription)
    : volumeName_((volumeDescription == nullptr) ? L"" : volumeDescription->VolumeName)
    , volumeDescription_(volumeDescription)
{
}

wstring const & StoreDataVolume::get_Type() const
{
    return Constants::StoreType_Volume;
}

wstring StoreDataVolume::ConstructKey() const
{
    return volumeName_;
}

void StoreDataVolume::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("StoreDataVolume({0})", volumeDescription_);
}
