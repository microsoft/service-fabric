// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreEnumeratorBase : public ReplicatedStoreEnumeratorBase
    {
        DENY_COPY(KeyValueStoreEnumeratorBase);

    public:
        KeyValueStoreEnumeratorBase(
            std::wstring const & keyPrefix,
            bool strictPrefix,
            IStoreBase::EnumerationSPtr const & replicatedStoreEnumeration);
        virtual ~KeyValueStoreEnumeratorBase();

    protected:
        Common::ErrorCode MoveNext();
        Common::ErrorCode TryMoveNext(bool & success);
        Common::ErrorCode OnMoveNextBase() override;

        virtual void ResetCurrent() = 0;

        virtual Common::ErrorCode InitializeCurrent(
            IStoreBase::EnumerationSPtr const &,
            std::wstring const & key,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            FILETIME lastModifiedUtc,
            FILETIME lastModifiedOnPrimaryUtc) = 0;

    private:
        std::wstring keyPrefix_;
        bool strictPrefix_;
        IStoreBase::EnumerationSPtr replicatedStoreEnumeration_;
    };
}
