// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace ServiceModel;

StoreDataComposeDeploymentInstance::StoreDataComposeDeploymentInstance()
    : deploymentName_()
    , version_()
    , composeFiles_()
    , sfSettingsFiles_()
    , repositoryUserName_()
    , repositoryUserPassword_()
    , isPasswordEncrypted_(false)
    , mergedComposeFile_()
{
}

StoreDataComposeDeploymentInstance::StoreDataComposeDeploymentInstance(
    wstring const & deploymentName,
    ServiceModelVersion const & version)
    : deploymentName_(deploymentName)
    , version_(version)
    , composeFiles_()
    , sfSettingsFiles_()
    , repositoryUserName_()
    , repositoryUserPassword_()
    , isPasswordEncrypted_(false)
    , mergedComposeFile_()
{
}

StoreDataComposeDeploymentInstance::StoreDataComposeDeploymentInstance(
    wstring const & deploymentName,
    ServiceModelVersion const & version,
    vector<ByteBuffer> && composeFiles,
    vector<ByteBuffer> && sfSettingsFiles,
    wstring const & repoUserName,
    wstring const & repoPassword,
    bool isPasswordEncrypted)
    : deploymentName_(deploymentName)
    , version_(version)
    , composeFiles_()
    , sfSettingsFiles_()
    , repositoryUserName_(repoUserName)
    , repositoryUserPassword_(repoPassword)
    , isPasswordEncrypted_(isPasswordEncrypted)
    , mergedComposeFile_()
{
    composeFiles_.reserve(composeFiles.size());
    for (int i = 0; i < composeFiles.size(); ++i)
    {
        composeFiles_.push_back(make_shared<ByteBuffer>(move(composeFiles[i])));
    }

    sfSettingsFiles_.reserve(sfSettingsFiles.size());
    for (int i = 0; i < sfSettingsFiles.size(); ++i)
    {
        sfSettingsFiles_.push_back(make_shared<ByteBuffer>(move(sfSettingsFiles[i])));
    }
}

StoreDataComposeDeploymentInstance::StoreDataComposeDeploymentInstance(
    StoreDataComposeDeploymentInstance && other)
    : deploymentName_(move(other.deploymentName_))
    , version_(move(other.version_))
    , composeFiles_(move(other.composeFiles_))
    , sfSettingsFiles_(move(other.sfSettingsFiles_))
    , repositoryUserName_(move(other.repositoryUserName_))
    , repositoryUserPassword_(move(other.repositoryUserPassword_))
    , isPasswordEncrypted_(move(other.isPasswordEncrypted_))
    , mergedComposeFile_(move(other.mergedComposeFile_))
{
}

wstring const & StoreDataComposeDeploymentInstance::get_Type() const
{
    return Constants::StoreType_ComposeDeploymentInstance;
}

wstring StoreDataComposeDeploymentInstance::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        this->GetDeploymentNameKeyPrefix(),
        version_);
    return temp;
}

std::wstring StoreDataComposeDeploymentInstance::GetDeploymentNameKeyPrefix() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        deploymentName_,
        Constants::TokenDelimeter);
    return temp;
}

void StoreDataComposeDeploymentInstance::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "StoreDataComposeDeploymentInstance({0}:{1}, ComposeFiles : {2}, sfSettingsFiles : {3}, repo user name ={4})", 
        deploymentName_, 
        version_, 
        composeFiles_.size(),
        sfSettingsFiles_.size(),
        repositoryUserName_);
}

ErrorCode StoreDataComposeDeploymentInstance::UpdateMergedComposeFile(
    Store::StoreTransaction &storeTx, 
    wstring const& mergedComposeFile)
{
    this->mergedComposeFile_ = mergedComposeFile;
    return storeTx.Update(*this);
}
