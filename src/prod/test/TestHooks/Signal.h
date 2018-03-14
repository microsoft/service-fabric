// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestHooks
{
    /*
        A signal object is created for each API when a test script tries to induce blocks
        The API calls the WaitForSignalReset method
        If the user has invoked a block this will block until the signal is reset and mark that someone is waiting
        If the user has not invoked a block this will return immediately

        The signal also supports waiting for the signal to be hit (i.e. after a block has been set waiting for it to be hit)

        The signal also supports waiting and ensuring that it is not hit in a certain timeout
    */
    class Signal : public Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
    {
        DENY_COPY(Signal);

    public:
        typedef std::function<void(std::wstring const &)> FailTestFunctionPtr;
        
        Signal(FailTestFunctionPtr failTestFunctionPtr, std::wstring const & id);

        // Waits until the signal is hit
        void WaitForSignalHit();

        // Fails if the signal is hit in a certain time
        void FailIfSignalHit(Common::TimeSpan timeout);

        // Waits until the signal is reset
        // This is called by the API
        void WaitForSignalReset();

        // Sets the block
        void SetSignal();

        // Removes the block
        void ResetSignal();

        bool IsSet();

    private:
        FailTestFunctionPtr failTestFunctionPtr_;
        std::wstring id_;
        Common::ManualResetEvent signalEvent_;
        Common::ManualResetEvent isSignalHitEvent_;
    };
}
