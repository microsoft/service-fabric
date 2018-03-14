// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

void SigUtil::BlockSignalOnCallingThread(int sig)
{
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, sig);

    auto retval = pthread_sigmask(SIG_BLOCK, &sset, nullptr);
    auto err = errno;
    ASSERT_IF(retval, "pthread_sigmask failed: retval = {0}, errno = {1}", retval, err);
}

void SigUtil::BlockAllFabricSignalsOnCallingThread()
{
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, FABRIC_SIGNO_TIMER);
    sigaddset(&sset, FABRIC_SIGNO_LEASE_TIMER);

    auto retval = pthread_sigmask(SIG_BLOCK, &sset, nullptr);
    auto err = errno;
    ASSERT_IF(retval, "pthread_sigmask failed: retval = {0}, errno = {1}", retval, err);
}
