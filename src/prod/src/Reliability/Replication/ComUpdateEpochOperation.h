// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Special type of operation that signifies 
        // that an UpdateEpoch should be called on the state provider.
        class ComUpdateEpochOperation : public ComOperation
        {
            DENY_COPY(ComUpdateEpochOperation)

            COM_INTERFACE_LIST2(
                ComUpdateEpochOperation,
                IID_IFabricOperation,
                IFabricOperation,
                CLSID_ComOperation,
                ComOperation)

        public:
                        
            virtual ~ComUpdateEpochOperation() {}

            __declspec (property(get=get_PreviousEpochSequenceNumber)) FABRIC_SEQUENCE_NUMBER PreviousEpochSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_PreviousEpochSequenceNumber() const { return previousEpochSequenceNumber_; }

            virtual HRESULT STDMETHODCALLTYPE GetData( 
                /* [out] */ ULONG *count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const **buffers)
            {
                if ((count == NULL) || (buffers == NULL)) { return Common::ComUtility::OnPublicApiReturn(E_POINTER); }
                *count = 0;
                *buffers = nullptr;
                return Common::ComUtility::OnPublicApiReturn(S_OK);
            }

            virtual HRESULT STDMETHODCALLTYPE Acknowledge() 
            {
                return Common::ComUtility::OnPublicApiReturn(S_OK);
            }

            virtual bool IsEmpty() const
            {
                return true;
            }

        private:
            ComUpdateEpochOperation(
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER previousEpochSequenceNumber)
                :   ComOperation(FABRIC_OPERATION_METADATA(), epoch, FABRIC_INVALID_SEQUENCE_NUMBER),
                    previousEpochSequenceNumber_(previousEpochSequenceNumber)
            {
            }

            FABRIC_SEQUENCE_NUMBER previousEpochSequenceNumber_;

            template <class ComImplementation,class T0,class T1>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation,class ComInterface,class T0,class T1>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1);
            
        }; // end ComUpdateEpochOperation

    } // end namespace ReplicationComponent
} // end namespace Reliability
