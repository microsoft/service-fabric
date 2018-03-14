// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Implementation of IFabricOperation that encapsulates IFabricOperationData provided by the user
        // Used on the primary for the copy and replication operations
        // and on the secondary for copy context operations.
        class ComUserDataOperation : public ComOperation
        {
            DENY_COPY(ComUserDataOperation)

            COM_INTERFACE_LIST2(
                ComUserDataOperation,
                IID_IFabricOperation,
                IFabricOperation,
                CLSID_ComOperation,
                ComOperation)
                
        public:

            virtual ~ComUserDataOperation();

            virtual HRESULT STDMETHODCALLTYPE GetData(
                /* [out] */ ULONG * count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const **buffers);

            virtual HRESULT STDMETHODCALLTYPE Acknowledge();

            virtual bool IsEmpty() const;

        private:
            ComUserDataOperation(
                Common::ComPointer<IFabricOperationData> && comOperationDataPointer,
                FABRIC_OPERATION_METADATA const & metadata);

            ComUserDataOperation(
                Common::ComPointer<IFabricOperationData> && comOperationDataPointer,
                FABRIC_OPERATION_METADATA const & metadata,
                FABRIC_EPOCH const & epoch);
            
            Common::ComPointer<IFabricOperationData> operation_;
            
            template <class ComImplementation,class T0,class T1>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation,class ComInterface,class T0,class T1>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation,class T0,class T1,class T2>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1, T2 && a2);

            template <class ComImplementation,class ComInterface,class T0,class T1,class T2>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2);

        }; // end ComUserDataOperation

    } // end namespace ReplicationComponent
} // end namespace Reliability
