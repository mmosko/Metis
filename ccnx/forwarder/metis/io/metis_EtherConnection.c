/*
 * Copyright (c) 2013-2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 * 
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */
/**
 * @author Marc Mosko, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2013-2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */

/**
 * Embodies the reader/writer for an Ethernet connection
 *
 * TODO: Put in a maintenance timer to timeout MAC entries if not used.  Need to add a function
 * the listener can call to bump up activity when a packet is received.
 *
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net/ethernet.h>

#include <LongBow/runtime.h>

#include <parc/algol/parc_Hash.h>
#include <parc/algol/parc_Memory.h>


#include <ccnx/forwarder/metis/core/metis_Forwarder.h>
#include <ccnx/forwarder/metis/core/metis_Connection.h>
#include <ccnx/forwarder/metis/core/metis_Message.h>
#include <ccnx/forwarder/metis/tlv/metis_Tlv.h>
#include <ccnx/forwarder/metis/io/metis_EtherConnection.h>
#include <ccnx/forwarder/metis/messenger/metis_MissiveType.h>
#include <ccnx/forwarder/metis/io/metis_HopByHopFragmenter.h>

typedef struct metis_ether_state {
    MetisForwarder *metis;
    MetisLogger *logger;

    // the udp listener socket we receive packets on
    MetisGenericEther *ether;

    MetisAddressPair *addressPair;

    MetisHopByHopFragmenter *fragmenter;

    // We need to access this all the time, so grab it out
    // of the addressPair;
    uint8_t myAddress[ETHER_ADDR_LEN];
    uint8_t peerAddress[ETHER_ADDR_LEN];
    uint16_t networkOrderEtherType;

    bool isUp;
    unsigned id;
} _MetisEtherState;

// Prototypes
static bool                     _metisEtherConnection_Send(MetisIoOperations *ops, const CPIAddress *nexthop, MetisMessage *message);
static const CPIAddress *_metisEtherConnection_GetRemoteAddress(const MetisIoOperations *ops);
static const MetisAddressPair *_metisEtherConnection_GetAddressPair(const MetisIoOperations *ops);
static unsigned                 _metisEtherConnection_GetConnectionId(const MetisIoOperations *ops);
static bool                     _metisEtherConnection_IsUp(const MetisIoOperations *ops);
static bool                     _metisEtherConnection_IsLocal(const MetisIoOperations *ops);
static void                     _metisEtherConnection_Release(MetisIoOperations **opsPtr);

static void                     _setConnectionState(_MetisEtherState *ether, bool isUp);
static CPIConnectionType        _metisEtherConnection_getConnectionType(const MetisIoOperations *ops);

/*
 * This assigns a unique pointer to the void * which we use
 * as a GUID for this class.
 */
static const void *_metisIoOperationsGuid = __FILE__;

/*
 * Return our GUID
 */
static const void *
_metisEtherConnection_Class(const MetisIoOperations *ops)
{
    return _metisIoOperationsGuid;
}

static MetisIoOperations _template = {
    .closure           = NULL,
    .send              = &_metisEtherConnection_Send,
    .getRemoteAddress  = &_metisEtherConnection_GetRemoteAddress,
    .getAddressPair    = &_metisEtherConnection_GetAddressPair,
    .getConnectionId   = &_metisEtherConnection_GetConnectionId,
    .isUp              = &_metisEtherConnection_IsUp,
    .isLocal           = &_metisEtherConnection_IsLocal,
    .destroy           = &_metisEtherConnection_Release,
    .class             = &_metisEtherConnection_Class,
    .getConnectionType = &_metisEtherConnection_getConnectionType
};

// =================================================================

static bool
_metisEtherConnection_FillInMacAddress(uint8_t *mac, const CPIAddress *source)
{
    PARCBuffer *macAddress = cpiAddress_GetLinkAddress(source);
    if (macAddress) {
        parcBuffer_GetBytes(macAddress, ETHER_ADDR_LEN, mac);
        parcBuffer_Rewind(macAddress);
        return true;
    }
    return false;
}

MetisIoOperations *
metisEtherConnection_Create(MetisForwarder *metis, MetisGenericEther *ether, MetisAddressPair *pair)
{
    bool success = false;
    size_t allocationSize = sizeof(_MetisEtherState) + sizeof(MetisIoOperations);
    MetisIoOperations *ops = parcMemory_AllocateAndClear(allocationSize);
    if (ops) {
        memcpy(ops, &_template, sizeof(MetisIoOperations));
        ops->closure = (uint8_t *) ops + sizeof(MetisIoOperations);

        _MetisEtherState *etherConnState = metisIoOperations_GetClosure(ops);

        if (_metisEtherConnection_FillInMacAddress(etherConnState->myAddress, metisAddressPair_GetLocal(pair))) {
            if (_metisEtherConnection_FillInMacAddress(etherConnState->peerAddress, metisAddressPair_GetRemote(pair))) {
                etherConnState->metis = metis;
                etherConnState->logger = metisLogger_Acquire(metisForwarder_GetLogger(metis));
                etherConnState->ether = metisGenericEther_Acquire(ether);
                etherConnState->id = metisForwarder_GetNextConnectionId(metis);
                etherConnState->addressPair = metisAddressPair_Acquire(pair);
                etherConnState->networkOrderEtherType = htons(metisGenericEther_GetEtherType(ether));
                etherConnState->fragmenter = metisHopByHopFragmenter_Create(etherConnState->logger, metisGenericEther_GetMTU(ether));

                _setConnectionState(etherConnState, true);

                if (metisLogger_IsLoggable(etherConnState->logger, MetisLoggerFacility_IO, PARCLogLevel_Debug)) {
                    char *str = metisAddressPair_ToString(pair);
                    metisLogger_Log(etherConnState->logger, MetisLoggerFacility_IO, PARCLogLevel_Debug, __func__,
                                    "EtherConnection %p created address pair %s",
                                    (void *) etherConnState, str);
                    free(str);
                }

                metisMessenger_Send(metisForwarder_GetMessenger(metis), metisMissive_Create(MetisMissiveType_ConnectionCreate, etherConnState->id));
                metisMessenger_Send(metisForwarder_GetMessenger(metis), metisMissive_Create(MetisMissiveType_ConnectionUp, etherConnState->id));

                success = true;
            }
        }
    }

    if (ops && !success) {
        if (metisLogger_IsLoggable(metisForwarder_GetLogger(metis), MetisLoggerFacility_IO, PARCLogLevel_Error)) {
            char *str = metisAddressPair_ToString(pair);
            metisLogger_Log(metisForwarder_GetLogger(metis), MetisLoggerFacility_IO, PARCLogLevel_Error, __func__,
                            "Error creating EtherConnection address pair %s",
                            str);
            free(str);
        }

        parcMemory_Deallocate(&ops);
    }

    return ops;
}

// =================================================================
// I/O Operations implementation

/*
 * We only do one allocation  of the combined MetisIoOperations and _MetisEtherState, so this
 * function only needs to destroy the internal state
 */
static void
_metisEtherConnection_InternalRelease(_MetisEtherState *etherConnState)
{
    if (metisLogger_IsLoggable(etherConnState->logger, MetisLoggerFacility_IO, PARCLogLevel_Debug)) {
        metisLogger_Log(etherConnState->logger, MetisLoggerFacility_IO, PARCLogLevel_Debug, __func__,
                        "EtherConnection %p destroyed",
                        (void *) etherConnState);
    }

    metisMessenger_Send(metisForwarder_GetMessenger(etherConnState->metis), metisMissive_Create(MetisMissiveType_ConnectionDestroyed, etherConnState->id));

    metisAddressPair_Release(&etherConnState->addressPair);
    metisGenericEther_Release(&etherConnState->ether);
    metisHopByHopFragmenter_Release(&etherConnState->fragmenter);
    metisLogger_Release(&etherConnState->logger);
}

static void
_metisEtherConnection_Release(MetisIoOperations **opsPtr)
{
    assertNotNull(opsPtr, "Parameter opsPtr must be non-null double pointer");
    assertNotNull(*opsPtr, "Parameter opsPtr must dereference to non-null pointer");

    MetisIoOperations *ops = *opsPtr;
    assertNotNull(metisIoOperations_GetClosure(ops), "ops->context must not be null");

    _MetisEtherState *etherConnState = (_MetisEtherState *) metisIoOperations_GetClosure(ops);
    _metisEtherConnection_InternalRelease(etherConnState);

    // do not close udp->udpListenerSocket, the listener will close
    // that when its done

    parcMemory_Deallocate((void **) &ops);
}

static bool
_metisEtherConnection_IsUp(const MetisIoOperations *ops)
{
    assertNotNull(ops, "Parameter must be non-null");
    const _MetisEtherState *etherConnState = (const _MetisEtherState *) metisIoOperations_GetClosure(ops);
    return etherConnState->isUp;
}

static bool
_metisEtherConnection_IsLocal(const MetisIoOperations *ops)
{
    return false;
}

static const CPIAddress *
_metisEtherConnection_GetRemoteAddress(const MetisIoOperations *ops)
{
    assertNotNull(ops, "Parameter must be non-null");
    const _MetisEtherState *etherConnState = (const _MetisEtherState *) metisIoOperations_GetClosure(ops);
    return metisAddressPair_GetRemote(etherConnState->addressPair);
}

static const MetisAddressPair *
_metisEtherConnection_GetAddressPair(const MetisIoOperations *ops)
{
    assertNotNull(ops, "Parameter must be non-null");
    const _MetisEtherState *etherConnState = (const _MetisEtherState *) metisIoOperations_GetClosure(ops);
    return etherConnState->addressPair;
}


static unsigned
_metisEtherConnection_GetConnectionId(const MetisIoOperations *ops)
{
    assertNotNull(ops, "Parameter must be non-null");
    const _MetisEtherState *etherConnState = (const _MetisEtherState *) metisIoOperations_GetClosure(ops);
    return etherConnState->id;
}

static bool
_sendFrame(_MetisEtherState *etherConnState, MetisMessage *message)
{
    PARCEventBuffer *writeBuffer = parcEventBuffer_Create();

    int failure = metisMessage_Append(writeBuffer, message);
    if (failure) {
        parcEventBuffer_Destroy(&writeBuffer);
        return false;
    }

    // Add an Ethernet header
    struct ether_header header;

    // Fill in the ethernet header
    header.ether_type = etherConnState->networkOrderEtherType;
    memcpy(header.ether_dhost, etherConnState->peerAddress, ETHER_ADDR_LEN);
    memcpy(header.ether_shost, etherConnState->myAddress, ETHER_ADDR_LEN);

    // and put it at the front of the output buffer
    parcEventBuffer_Prepend(writeBuffer, &header, sizeof(header));

    bool success = metisGenericEther_SendFrame(etherConnState->ether, writeBuffer);

    // we're done with the buffer
    parcEventBuffer_Destroy(&writeBuffer);

    // BugzID: 3343 - close the connection on certain errors??
    return success;
}

/**
 * @function metisEtherConnection_Send
 * @abstract Non-destructive send of the message.
 * @discussion
 *   sends a message to the peer.
 *
 * @param dummy is ignored.  A udp connection has only one peer.
 * @return <#return#>
 */
static bool
_metisEtherConnection_Send(MetisIoOperations *ops, const CPIAddress *dummy, MetisMessage *message)
{
    assertNotNull(ops, "Parameter ops must be non-null");
    assertNotNull(message, "Parameter message must be non-null");
    _MetisEtherState *etherConnState = (_MetisEtherState *) metisIoOperations_GetClosure(ops);

    bool success = metisHopByHopFragmenter_Send(etherConnState->fragmenter, message);

    MetisMessage *fragment;
    while (success && (fragment = metisHopByHopFragmenter_PopSendQueue(etherConnState->fragmenter)) != NULL) {
        success = _sendFrame(etherConnState, fragment);
        metisMessage_Release(&fragment);
    }

    // if we failed, drain the other fragments
    if (!success) {
        while ((fragment = metisHopByHopFragmenter_PopSendQueue(etherConnState->fragmenter)) != NULL) {
            metisMessage_Release(&fragment);
        }
    }

    return success;
}

static void
_setConnectionState(_MetisEtherState *etherConnState, bool isUp)
{
    assertNotNull(etherConnState, "Parameter Udp must be non-null");

    MetisMessenger *messenger = metisForwarder_GetMessenger(etherConnState->metis);

    bool oldStateIsUp = etherConnState->isUp;
    etherConnState->isUp = isUp;

    if (oldStateIsUp && !isUp) {
        // bring connection DOWN
        MetisMissive *missive = metisMissive_Create(MetisMissiveType_ConnectionDown, etherConnState->id);
        metisMessenger_Send(messenger, missive);
        return;
    }


    if (!oldStateIsUp && isUp) {
        // bring connection UP
        MetisMissive *missive = metisMissive_Create(MetisMissiveType_ConnectionUp, etherConnState->id);
        metisMessenger_Send(messenger, missive);
        return;
    }
}

static CPIConnectionType
_metisEtherConnection_getConnectionType(const MetisIoOperations *ops)
{
    return cpiConnection_L2;
}


bool
metisEtherConnection_IsInstanceOf(const MetisConnection *conn)
{
    bool result = false;
    if (conn != NULL) {
        const void *class = metisConnection_Class(conn);
        if (class == _metisIoOperationsGuid) {
            result = true;
        }
    }
    return result;
}


MetisHopByHopFragmenter *
metisEtherConnection_GetFragmenter(const MetisConnection *conn)
{
    MetisHopByHopFragmenter *fragmenter = NULL;

    if (metisEtherConnection_IsInstanceOf(conn)) {
        MetisIoOperations *ops = metisConnection_GetIoOperations(conn);
        _MetisEtherState *state = (_MetisEtherState *) metisIoOperations_GetClosure(ops);
        fragmenter = state->fragmenter;
    }
    return fragmenter;
}

