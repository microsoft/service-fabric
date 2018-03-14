// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef TESTFABRICASYNCOPERATIONCALLBACKHELPER_H
#define TESTFABRICASYNCOPERATIONCALLBACKHELPER_H

#include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    class TestFabricAsyncCallbackHelper
        : public IFabricAsyncOperationCallback
        // IFabricAsyncOperationCallback derives from IUnknown
        , private Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(TestFabricAsyncCallbackHelper, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

    public:
        TestFabricAsyncCallbackHelper(std::function<void(IFabricAsyncOperationContext *)> const &);

        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *);

    private:
        std::function<void(IFabricAsyncOperationContext *)> callback_;
    };
}

#endif
