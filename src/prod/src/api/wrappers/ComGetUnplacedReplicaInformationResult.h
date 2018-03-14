// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {C5B93A86-7E45-4375-8CA0-AEA2695A8F19}
    static const GUID CLSID_ComGetUnplacedReplicaInformationResult =
    { 0xc5b93a86, 0x7e45, 0x4375, { 0x8c, 0xa0, 0xae, 0xa2, 0x69, 0x5a, 0x8f, 0x19 } };

    class ComGetUnplacedReplicaInformationResult :
        public IFabricGetUnplacedReplicaInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetUnplacedReplicaInformationResult)

        COM_INTERFACE_LIST2(
            ComGetUnplacedReplicaInformationResult,
            IID_IFabricGetUnplacedReplicaInformationResult,
            IFabricGetUnplacedReplicaInformationResult,
            CLSID_ComGetUnplacedReplicaInformationResult,
            ComGetUnplacedReplicaInformationResult)

    public:
        explicit ComGetUnplacedReplicaInformationResult(ServiceModel::UnplacedReplicaInformationQueryResult const & unplacedReplicaInformation);

        //
        // IFabricGetUnplacedReplicaInformationResult methods
        //
        const FABRIC_UNPLACED_REPLICA_INFORMATION *STDMETHODCALLTYPE get_UnplacedReplicaInformation( void);

    private:
        Common::ReferencePointer<FABRIC_UNPLACED_REPLICA_INFORMATION> unplacedReplicaInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetUnplacedReplicaInformationResult> ComGetUnplacedReplicaInformationResultCPtr;
}
