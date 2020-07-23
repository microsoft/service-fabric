// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef COMPATFLAGDEF
#error You must define COMPATFLAGDEF macro before including compatibilityflagsdef.h
#endif

COMPATFLAGDEF(SwallowUnhandledExceptions) // Legacy exception handling policy - swallow unhandled exceptions

COMPATFLAGDEF(NullReferenceExceptionOnAV) // Legacy null reference exception policy - throw NullReferenceExceptions for access violations

COMPATFLAGDEF(EagerlyGenerateRandomAsymmKeys) // Legacy mode for DSACryptoServiceProvider/RSACryptoServiceProvider - create a random key in the constructor eagerly

COMPATFLAGDEF(FullTrustListAssembliesInGac) // Legacy mode for not requiring FT list assemblies to be in the GAC - if set, the requirement to be in the GAC would not be enforced.

COMPATFLAGDEF(DateTimeParseIgnorePunctuation) // Through to V1.1, DateTime parse would ignore any unrecognized punctuation. 
                                              // This flag restores that behavior.

COMPATFLAGDEF(OnlyGACDomainNeutral)         // This allows late setting of app domain security and 
                                            // assembly evidence, even when LoaderOptimization=MultiDomain

COMPATFLAGDEF(DisableReplacementCustomCulture) // This allow disabling replacement custom cultures. will always get the shipped framework culture.

#undef COMPATFLAGDEF
