// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class BackupVersion final
    {

    public:
        static BackupVersion Invalid();

        static LONG32 Compare(
            __in BackupVersion const & itemOne,
            __in BackupVersion const & itemTwo);

    public:
        __declspec(property(get = get_Epoch)) FABRIC_EPOCH Epoch;
        FABRIC_EPOCH get_Epoch() const;

        __declspec(property(get = get_LSN)) FABRIC_SEQUENCE_NUMBER LSN;
        FABRIC_SEQUENCE_NUMBER get_LSN() const;

    public:
        BackupVersion();

        BackupVersion(
            __in FABRIC_EPOCH epoch,
            __in FABRIC_SEQUENCE_NUMBER lsn);

        BackupVersion & operator=(
            __in const BackupVersion & other);

    public:
        bool operator==(__in BackupVersion const & other) const;
        bool operator!=(__in BackupVersion const & other) const;
        bool operator>(__in BackupVersion const & other) const;
        bool operator<(__in BackupVersion const & other) const;
        bool operator>=(__in BackupVersion const & other) const;
        bool operator<=(__in BackupVersion const & other) const;

    private:
        FABRIC_EPOCH epoch_;
        FABRIC_SEQUENCE_NUMBER lsn_;
    };
}
