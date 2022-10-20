// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef LOCATIONINFO_H
#define LOCATIONINFO_H


// in order of preference, smaller is better
enum LocationInfo
{
    Loc_System=1,
    Loc_Machine=2,
    Loc_User=3,
    Loc_Network=4,
    Loc_Undefined =0xffff
};

// Returns the more preferred of two locations
//
// Assumptions: LocationInfo is defined in a manner that a smaller value is better
//
// Input:
// locations to compare
//
// Output: 
// the preferred location
inline LocationInfo BetterLocation(LocationInfo l1, LocationInfo l2)
{
    return l1<l2?l1:l2;
};

#endif //  LOCATIONINFO_H
