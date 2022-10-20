// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReplicationUnitTest
{
    class ComTestBatchOperation
        : public IFabricBatchOperationData
        , private Common::ComUnknownBase
        ,  public Common::TextTraceComponent<Common::TraceTaskCodes::Replication>
    {
        DENY_COPY(ComTestBatchOperation)

        COM_INTERFACE_LIST1(
            ComTestBatchOperation,
            IID_IFabricBatchOperationData,
            IFabricBatchOperationData)

    public:

        explicit ComTestBatchOperation()
            : IFabricBatchOperationData(),
              ComUnknownBase(),
              firstSequenceNumber_(Reliability::ReplicationComponent::Constants::InvalidLSN),
              lastSequenceNumber_(Reliability::ReplicationComponent::Constants::InvalidLSN),
              operation1_(),
              operation2_(),
              operation3_()
        {
        }

        explicit ComTestBatchOperation(
            FABRIC_SEQUENCE_NUMBER firstSequenceNumber,
            FABRIC_SEQUENCE_NUMBER lastSequenceNumber,
            std::wstring const descriptions[])
            : IFabricBatchOperationData(),
              ComUnknownBase(),
              firstSequenceNumber_(firstSequenceNumber),
              lastSequenceNumber_(lastSequenceNumber),
              operation1_(),
              operation2_(),
              operation3_()
        {
            operation1_ = Common::make_com<ComTestOperation, IFabricOperationData>(descriptions[0], descriptions[1]);
            operation2_ = Common::make_com<ComTestOperation, IFabricOperationData>(descriptions[2]);
            operation3_ = Common::make_com<ComTestOperation, IFabricOperationData>(descriptions[3], descriptions[4]);
        }

        virtual ~ComTestBatchOperation()
        {
        }

        FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_FirstSequenceNumber()
        {
            return firstSequenceNumber_;
        }

        FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_LastSequenceNumber()
        {
            return lastSequenceNumber_;
        }

        HRESULT STDMETHODCALLTYPE GetData(
            FABRIC_SEQUENCE_NUMBER sequenceNumber, 
            ULONG * count,
            FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
            switch (sequenceNumber - firstSequenceNumber_)
            {
            case 0:
                return operation1_->GetData(count, buffers);

            case 1:
                return operation2_->GetData(count, buffers);

            case 2:
                return operation3_->GetData(count, buffers);

            default:
                return E_INVALIDARG;
            }
        }

    private:
        FABRIC_SEQUENCE_NUMBER firstSequenceNumber_;
        FABRIC_SEQUENCE_NUMBER lastSequenceNumber_;
        Common::ComPointer<IFabricOperationData> operation1_;
        Common::ComPointer<IFabricOperationData> operation2_;
        Common::ComPointer<IFabricOperationData> operation3_;
    };
}
