// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {0103E744-1B4A-41CA-8277-BA1147EF53FF}
    static const GUID CLSID_ComGetTestCommandStatusResult = 
    {0x103e744, 0x1b4a, 0x41ca, { 0x82, 0x77, 0xba, 0x11, 0x47, 0xef, 0x53, 0xff}}; 
    class ComGetTestCommandStatusResult :
    public IFabricTestCommandStatusResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetTestCommandStatusResult)

        COM_INTERFACE_LIST2(
            ComGetTestCommandStatusResult,
            IID_IFabricTestCommandStatusResult,
            IFabricTestCommandStatusResult,
            CLSID_ComGetTestCommandStatusResult,
            ComGetTestCommandStatusResult)
    public:
        explicit ComGetTestCommandStatusResult(std::vector<ServiceModel::TestCommandListQueryResult> && result);

        const TEST_COMMAND_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_Result( void);

    private:
        Common::ReferencePointer<TEST_COMMAND_QUERY_RESULT_LIST> result_;

        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetTestCommandStatusResult> ComGetTestComamndStatusResultCPtr;
}
