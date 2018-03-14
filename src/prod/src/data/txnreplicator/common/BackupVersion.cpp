// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;

BackupVersion BackupVersion::Invalid()
{
    return BackupVersion();
}

LONG32 BackupVersion::Compare(
    __in BackupVersion const & itemOne,
    __in BackupVersion const & itemTwo)
{
    if (itemOne == itemTwo)
    {
        return 0;
    }

    if (itemOne < itemTwo)
    {
        return -1;
    }

    return 1;
}

BackupVersion::BackupVersion()
    : epoch_()
    , lsn_(FABRIC_INVALID_SEQUENCE_NUMBER)
{
    epoch_.DataLossNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
    epoch_.ConfigurationNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
}

BackupVersion::BackupVersion(
    __in FABRIC_EPOCH epoch,
    __in FABRIC_SEQUENCE_NUMBER lsn)
    : epoch_(epoch)
    , lsn_(lsn)
{
}

BackupVersion & BackupVersion::operator=(
    __in const BackupVersion & other)
{
    if (&other == this)
    {
        return *this;
    }

    epoch_ = other.epoch_;
    lsn_ = other.lsn_;

    return *this;
}

FABRIC_EPOCH BackupVersion::get_Epoch() const
{
    return epoch_;
} 

FABRIC_SEQUENCE_NUMBER BackupVersion::get_LSN() const
{
    return lsn_;
}

bool BackupVersion::operator==(BackupVersion const & other) const
{
    return (other.Epoch.DataLossNumber == epoch_.DataLossNumber) && 
        (other.Epoch.ConfigurationNumber == epoch_.ConfigurationNumber) && 
        (other.LSN == lsn_);
}

bool BackupVersion::operator!=(BackupVersion const & other) const
{
    return (other.Epoch.DataLossNumber != epoch_.DataLossNumber) ||
        (other.Epoch.ConfigurationNumber != epoch_.ConfigurationNumber) ||
        (other.LSN != lsn_);
}

bool BackupVersion::operator>(BackupVersion const & other) const
{
    if (epoch_.DataLossNumber > other.Epoch.DataLossNumber)
    {
        return true;
    }

    if (epoch_.DataLossNumber != other.Epoch.DataLossNumber)
    {
        return false;
    }

    if (epoch_.ConfigurationNumber > other.Epoch.ConfigurationNumber)
    {
        return true;
    }

    if (epoch_.ConfigurationNumber != other.Epoch.ConfigurationNumber)
    {
        return false;
    }

    return lsn_ > other.LSN;
}

bool BackupVersion::operator<(BackupVersion const & other) const
{
    if (epoch_.DataLossNumber < other.Epoch.DataLossNumber)
    {
        return true;
    }

    if (epoch_.DataLossNumber != other.Epoch.DataLossNumber)
    {
        return false;
    }

    if (epoch_.ConfigurationNumber < other.Epoch.ConfigurationNumber)
    {
        return true;
    }

    if (epoch_.ConfigurationNumber != other.Epoch.ConfigurationNumber)
    {
        return false;
    }

    return lsn_ < other.LSN;
}

bool BackupVersion::operator>=(BackupVersion const & other) const
{
    return (*this > other) || (*this == other);
}

bool BackupVersion::operator<=(BackupVersion const & other) const
{
    return (*this < other) || (*this == other);
}
