// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fticonfig_table_parse_rtm.cc,v 1.6 2004/03/17 07:33:32 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <net/route.h>

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"
#include "routing_socket_utils.hh"


//
// Parse information about routing table information received from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//
// Reading route(4) manual page is a good start for understanding this
//

#ifndef HAVE_ROUTING_SOCKETS
bool
FtiConfigTableGet::parse_buffer_rtm(int, list<FteX>& , const uint8_t* ,
				    size_t , bool )
{
    return false;
}

#else // HAVE_ROUTING_SOCKETS

bool
FtiConfigTableGet::parse_buffer_rtm(int family, list<FteX>& fte_list,
				    const uint8_t* buf, size_t buf_bytes,
				    bool is_rtm_get_only)
{
    const struct rt_msghdr* rtm = reinterpret_cast<const struct rt_msghdr *>(buf);
    const uint8_t* last = buf + buf_bytes;
    
    for (const uint8_t* ptr = buf; ptr < last; ptr += rtm->rtm_msglen) {
    	rtm = reinterpret_cast<const struct rt_msghdr *>(ptr);
	if (RTM_VERSION != rtm->rtm_version) {
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       rtm->rtm_version);
	    continue;
	}

	if (is_rtm_get_only) {
	    //
	    // Consider only the RTM_GET entries.
	    //
	    if (rtm->rtm_type != RTM_GET)
		continue;
	    if (! (rtm->rtm_flags & RTF_UP))
		continue;
	}

	if ((rtm->rtm_type != RTM_ADD)
	    && (rtm->rtm_type != RTM_DELETE)
	    && (rtm->rtm_type != RTM_CHANGE)
	    && (rtm->rtm_type != RTM_GET)) {
	    continue;
	}

	if (rtm->rtm_errno != 0)
	    continue;		// XXX: ignore entries with an error

#ifdef RTF_WASCLONED
	if (rtm->rtm_flags & RTF_WASCLONED)
	    continue;		// XXX: ignore cloned entries
#endif
#ifdef RTF_CLONED
	if (rtm->rtm_flags & RTF_CLONED)
	    continue;		// XXX: ignore cloned entries
#endif
#ifdef RTF_MULTICAST
	if (rtm->rtm_flags & RTF_MULTICAST)
	    continue;		// XXX: ignore multicast entries
#endif
#ifdef RTF_BROADCAST
	if (rtm->rtm_flags & RTF_BROADCAST)
	    continue;		// XXX: ignore broadcast entries
#endif
	
	FteX fte(family);
	if (RtmUtils::rtm_get_to_fte_cfg(fte, rtm) == true)
	    fte_list.push_back(fte);
    }
    
    return true;
}

#endif // HAVE_ROUTING_SOCKETS
