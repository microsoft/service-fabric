// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __HOLDERINST_H_
#define __HOLDERINST_H_

// This file contains holder instantiations which we can't put in holder.h because
// the instantiations require _ASSERTE to be defined, which is not always the case
// for placed that include holder.h.

FORCEINLINE void SafeArrayRelease(SAFEARRAY* p)
{
    SafeArrayDestroy(p);
}


class SafeArrayHolder : public Wrapper<SAFEARRAY*, SafeArrayDoNothing, SafeArrayRelease, NULL>
{
public:
    SafeArrayHolder(SAFEARRAY* p = NULL)
        : Wrapper<SAFEARRAY*, SafeArrayDoNothing, SafeArrayRelease, NULL>(p)
    {
    }

    FORCEINLINE void operator=(SAFEARRAY* p)
    {
        Wrapper<SAFEARRAY*, SafeArrayDoNothing, SafeArrayRelease, NULL>::operator=(p);
    }
};

#endif  // __HOLDERINST_H_
