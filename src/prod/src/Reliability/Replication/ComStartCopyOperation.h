// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Special type of operation that signifies StartCopy.
        // It contains the replication start sequence number
        class ComStartCopyOperation : public ComOperation
        {
            DENY_COPY(ComStartCopyOperation)

            COM_INTERFACE_LIST2(
                ComStartCopyOperation,
                IID_IFabricOperation,
                IFabricOperation,
                CLSID_ComOperation,
                ComOperation)

        public:
                        
            virtual ~ComStartCopyOperation() {}

            virtual HRESULT STDMETHODCALLTYPE GetData( 
                /* [out] */ ULONG *count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const **buffers)
            {
                if ((count == NULL) || (buffers == NULL)) { return Common::ComUtility::OnPublicApiReturn(E_POINTER); }
                
                *count = 0;
                *buffers = NULL;
                
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
            ComStartCopyOperation(FABRIC_OPERATION_METADATA const & metadata)
                :   ComOperation(metadata, Constants::InvalidEpoch, FABRIC_INVALID_SEQUENCE_NUMBER)
            {
            }

            template <class ComImplementation,class T0>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0);

            template <class ComImplementation,class ComInterface,class T0>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0);
            
        }; // end ComStartCopyOperation

    } // end namespace ReplicationComponent
} // end namespace Reliability
