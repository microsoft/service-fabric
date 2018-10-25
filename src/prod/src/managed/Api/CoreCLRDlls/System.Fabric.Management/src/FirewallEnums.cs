// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace NetFwTypeLib
{
    internal enum NET_FW_IP_PROTOCOL_
    {
        NET_FW_IP_PROTOCOL_ANY,
        NET_FW_IP_PROTOCOL_TCP,
        NET_FW_IP_PROTOCOL_UDP,
    }

    internal enum NET_FW_RULE_DIRECTION_
    {
        NET_FW_RULE_DIR_IN,
        NET_FW_RULE_DIR_OUT,
        NET_FW_RULE_DIR_MAX,
    }

    internal enum NET_FW_PROFILE_TYPE2_
    {
        NET_FW_PROFILE2_PUBLIC,
        NET_FW_PROFILE2_PRIVATE,
        NET_FW_PROFILE2_DOMAIN,
        NET_FW_PROFILE2_ALL,
    }

    internal enum NET_FW_ACTION_
    {
        NET_FW_ACTION_ALLOW,
        NET_FW_ACTION_BLOCK,
        NET_FW_ACTION_MAX,
    }
}