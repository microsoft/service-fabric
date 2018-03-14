// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStateProvider
    {
    public:
        virtual ~IStateProvider() {};

        virtual Common::AsyncOperationSPtr BeginUpdateEpoch( 
            FABRIC_EPOCH const & epoch,
            FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) = 0;

        virtual Common::ErrorCode EndUpdateEpoch(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::ErrorCode GetLastCommittedSequenceNumber( 
            __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) = 0;

        virtual Common::AsyncOperationSPtr BeginOnDataLoss(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) = 0;

        virtual Common::ErrorCode EndOnDataLoss(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) = 0;

        // TODO: change from ComPointer to IOperationDataStreamPtr
        virtual Common::ErrorCode GetCopyContext(
            __out Common::ComPointer<::IFabricOperationDataStream> & copyContextStream) = 0;

        // TODO: change from ComPointer to IOperationDataStreamPtr
        virtual Common::ErrorCode GetCopyState(
            ::FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            Common::ComPointer<::IFabricOperationDataStream> const & copyContextStream, 
            Common::ComPointer<::IFabricOperationDataStream> & copyStateStream) = 0;

    public:

        //
        // Optional test hooks
        //

        virtual Common::ErrorCode Test_GetBeginUpdateEpochInjectedFault() { return Common::ErrorCodeValue::Success; }
    };
}
