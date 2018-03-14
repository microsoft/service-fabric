// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class that exposes an IFabricOperation as IFabricOperationData
        // {444F6628-F01C-424E-BFC5-8F99DAE37F4C}

        static const GUID CLSID_ComOperationData =
            { 0x444f6628, 0xf01c, 0x424e, { 0xbf, 0xc5, 0x8f, 0x99, 0xda, 0xe3, 0x7f, 0x4c } };
        class ComOperationData : public IFabricOperationData, public Common::ComUnknownBase
        {
            DENY_COPY(ComOperationData)

            COM_INTERFACE_LIST1(
                ComOperationData,
                IID_IFabricOperationData,
                IFabricOperationData)
                
        public:

            virtual HRESULT STDMETHODCALLTYPE GetData( 
                /* [out] */ ULONG *count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const **buffers) 
            {
                if ((count == NULL) || (buffers == NULL)) { return Common::ComUtility::OnPublicApiReturn(E_POINTER); }
                
                if (!operation_)
                {
                    *count = 0;
                    return Common::ComUtility::OnPublicApiReturn(S_OK);
                }

                return Common::ComUtility::OnPublicApiReturn(operation_->GetData(count, buffers));
            }

        private:
            explicit ComOperationData(ComOperationCPtr && operation)
                : operation_(std::move(operation))
            {
            }

            template <class ComImplementation,class T0>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0);

            template <class ComImplementation,class ComInterface,class T0>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0);

            ComOperationCPtr operation_;
        }; // end ComOperationData

    } // end namespace ReplicationComponent
} // end namespace Reliability
