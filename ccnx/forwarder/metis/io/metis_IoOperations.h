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
 * Defines the interface all connections use to communicate with the forwarder.
 *
 * @code
 *
 *   static MetisIoOperations _template = {
 *      .closure           = NULL,
 *      .send              = &_metisEtherConnection_Send,
 *      .getRemoteAddress  = &_metisEtherConnection_GetRemoteAddress,
 *      .getAddressPair    = &_metisEtherConnection_GetAddressPair,
 *      .getConnectionId   = &_metisEtherConnection_GetConnectionId,
 *      .isUp              = &_metisEtherConnection_IsUp,
 *      .isLocal           = &_metisEtherConnection_IsLocal,
 *      .destroy           = &_metisEtherConnection_DestroyOperations,
 *      .class             = &_metisEtherConnection_Class,
 *      .getConnectionType = &_metisEtherConnection_getConnectionType
 *   };
 *
 *   MetisIoOperations *
 *   metisEtherConnection_Create(MetisForwarder *metis, MetisGenericEther *ether, MetisAddressPair *pair)
 *   {
 *      _MetisEtherState *etherConnState = parcMemory_Allocate(sizeof(_MetisEtherState));
 *      // Fill in etherConnState with instance variables
 *
 *      MetisIoOperations *io_ops = parcMemory_Allocate(sizeof(MetisIoOperations));
 *      memcpy(io_ops, &_template, sizeof(MetisIoOperations));
 *      io_ops->closure = etherConnState;
 *      // Add to connection table, send missives about connection state
 *
 *      return op_ops;
 *   }
 * @endcode
 *
 * @author Marc Mosko, Palo Alto Research Center (Xerox PARC)
 * @copyright (c) 2013-2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
 */

/**
 * I/O is built around a callback structure.  The connection table contains an operations
 * structure built around function pointers.  These allow the connection table to be
 * agnostic about underlying connections.
 */

#ifndef Metis_metis_io_h
#define Metis_metis_io_h

#include <ccnx/api/control/cpi_Connection.h>
#include <ccnx/api/control/cpi_Address.h>
#include <ccnx/forwarder/metis/io/metis_AddressPair.h>
#include <ccnx/forwarder/metis/core/metis_Message.h>

struct metis_io_ops;
typedef struct metis_io_ops MetisIoOperations;

/**
 * @typedef MetisIoOperations
 * @abstract The IO Operations structure abstracts an connection's properties and send() method
 * @constant context Implementation specific opaque data, passed back on each call
 * @constant send function pointer to send a message, does not destroy the message
 * @constant getRemoteAddress function pointer to return the "to" address associated with the connection.
 *           Some connections might not have a specific peer, such as multicast, where its the group address.
 * @constant isUp test if the connection is up, ready to send a message.
 * @constant isLocal test if the connection is local to the host.
 * @constant getConnectionId returns the Metis id for the connection.
 * @constant destroy releases a refernce count on the connection and possibly destroys the connection.
 * @constant class A unique identifier for each class that instantiates MetisIoOperations.
 * @constant getConnectionType Returns the type of connection (TCP, UDP, L2, etc.) of the underlying connection.
 * @discussion <#Discussion#>
 */
struct metis_io_ops {
    void *closure;
    bool (*send)(MetisIoOperations *ops, const CPIAddress *nexthop, MetisMessage *message);
    const CPIAddress *       (*getRemoteAddress)(const MetisIoOperations *ops);
    const MetisAddressPair * (*getAddressPair)(const MetisIoOperations *ops);
    bool (*isUp)(const MetisIoOperations *ops);
    bool (*isLocal)(const MetisIoOperations *ops);
    unsigned (*getConnectionId)(const MetisIoOperations *ops);
    void (*destroy)(MetisIoOperations **opsPtr);
    const void * (*class)(const MetisIoOperations *ops);
    CPIConnectionType (*getConnectionType)(const MetisIoOperations *ops);
};

/**
 * Returns the closure of the interface
 *
 * The creator of the closure sets this parameter to store its state.
 *
 * @param [in] ops A concrete instance of the interface
 *
 * @return The value set by the concrete instance of the interface.
 *
 * Example:
 * @clode
 * {

 * }
 * @endcode
 */
void *metisIoOperations_GetClosure(const MetisIoOperations *ops);

/**
 * Release all memory related to the interface and implementation
 *
 * This function must release all referenced memory in the concrete implementation and
 * memory related to the MetisIoOperations.  It should NULL the input parameter.
 *
 * @param [in,out] opsPtr Pointer to interface.  Will be NULLed.
 *
 * Example:
 * @code
 *
 *   static void
 *   _metisEtherConnection_InternalRelease(_MetisEtherState *etherConnState)
 *   {
 *      // release internal state of _MetisEtherState
 *   }
 *
 *   static void
 *   _metisEtherConnection_Release(MetisIoOperations **opsPtr)
 *   {
 *      MetisIoOperations *ops = *opsPtr;
 *
 *      _MetisEtherState *etherConnState = (_MetisEtherState *) metisIoOperations_GetClosure(ops);
 *      _metisEtherConnection_InternalRelease(etherConnState);
 *
 *      parcMemory_Deallocate((void **) &ops);
 *   }
 *
 *   MetisIoOperations *
 *   metisEtherConnection_Create(MetisForwarder *metis, MetisGenericEther *ether, MetisAddressPair *pair)
 *   {
 *      size_t allocationSize = sizeof(_MetisEtherState) + sizeof(MetisIoOperations);
 *      MetisIoOperations *ops = parcMemory_AllocateAndClear(allocationSize);
 *      if (ops) {
 *         // fill in other interface functions
 *         ops->destroy = &_metisEtherConnection_Release;
 *         ops->closure = (uint8_t *) ops + sizeof(MetisIoOperations);
 *
 *         _MetisEtherState *etherConnState = metisIoOperations_GetClosure(ops);
 *         // fill in Ethernet state
 *      }
 *      return ops;
 *   }
 * @endcode
 */
void metisIoOperations_Release(MetisIoOperations **opsPtr);

/**
 * Sends the specified MetisMessage out this connection
 *
 * The the implementation of send may queue the message, it must acquire a reference to it.
 *
 * @param [in] ops The connection implementation.
 * @param [in] nexthop On multiple access networks, this parameter might be used, usually NULL.
 * @param [in] message The message to send.  If the message will be queued, it will be acquired.
 *
 * @return true The message was sent or queued
 * @retrun false An error occured and the message will not be sent or queued
 *
 * Example:
 * @code
 * {
 *     if (metisIoOperations_IsUp(conn->ops)) {
 *        return metisIoOperations_Send(conn->ops, NULL, message);
 *     }
 * }
 * @endcode
 */
bool metisIoOperations_Send(MetisIoOperations *ops, const CPIAddress *nexthop, MetisMessage *message);

/**
 * A connection is made up of a local and a remote address.  This function returns the remote address.
 *
 * Represents the destination endpoint of the communication.
 *
 * @param [in] ops The connection implementation.
 *
 * @return non-null The remote address
 * @return null The connection does not have a remote address
 *
 * Example:
 * @code
 * {
 *    CPIAddress *local =  cpiAddress_CreateFromLink((uint8_t []) { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, 6);
 *    CPIAddress *remote = cpiAddress_CreateFromLink((uint8_t []) { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }, 6);
 *    MetisAddressPair *pair = metisAddressPair_Create(local, remote);
 *    MetisIoOperations *ops = metisEtherConnection_Create(metis, ether, pair);
 *
 *    const CPIAddress *test = metisIoOperations_GetRemoteAddress(ops);
 *    assertTrue(cpiAddress_Equals(test, remote), "Wrong remote address");
 *    metisIoOperations_Release(&ops);
 *    metisAddressPair_Release(&pair);
 *    cpiAddress_Destroy(&local);
 *    cpiAddress_Destroy(&remote);
 * }
 * @endcode
 */
const CPIAddress *metisIoOperations_GetRemoteAddress(const MetisIoOperations *ops);

/**
 * A connection is made up of a local and a remote address.  This function returns the address pair.
 *
 * Represents the destination endpoint of the communication.
 *
 * @param [in] ops The connection implementation.
 *
 * @return non-null The address pair
 * @return null An error.
 *
 * Example:
 * @code
 * {
 *    CPIAddress *local =  cpiAddress_CreateFromLink((uint8_t []) { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, 6);
 *    CPIAddress *remote = cpiAddress_CreateFromLink((uint8_t []) { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }, 6);
 *    MetisAddressPair *pair = metisAddressPair_Create(local, remote);
 *    MetisIoOperations *ops = metisEtherConnection_Create(metis, ether, pair);
 *
 *    const MetisAddressPair *test = metisIoOperations_GetAddressPair(ops);
 *    assertTrue(metisAddressPair(test, pair), "Wrong address pair");
 *    metisIoOperations_Release(&ops);
 *    metisAddressPair_Release(&pair);
 *    cpiAddress_Destroy(&local);
 *    cpiAddress_Destroy(&remote);
 * }
 * @endcode
 */
const MetisAddressPair *metisIoOperations_GetAddressPair(const MetisIoOperations *ops);

/**
 * Returns true if the underlying connection is in operation
 *
 * An UP connection is able to send and receive packets. If a subsystem needs to take actions
 * when a connection goes UP or DOWN, it should subscribe as a MetisMissive listener.
 *
 * @param [in] ops The connection implementation.
 *
 * @return true The connection is UP
 * @return false The connection is not UP
 *
 * Example:
 * @code
 * {
 *     if (metisIoOperations_IsUp(conn->ops)) {
 *        return metisIoOperations_Send(conn->ops, NULL, message);
 *     }
 * }
 * @endcode
 */
bool metisIoOperations_IsUp(const MetisIoOperations *ops);

/**
 * If the remote address is local to this system, returns true
 *
 * Will return true if an INET or INET6 connection is on localhost.  Will return
 * true for AF_UNIX.  An Ethernet connection is not local.
 *
 * @param [in] ops The connection implementation.
 *
 * @return true The remote address is local to the system
 * @return false The remote address is not local
 *
 * Example:
 * @code
 * {
 *     // Is the ingress connection remote?  If so check for non-zero and decrement
 *     if (!metisIoOperations(ingressConnectionOps) {
 *        uint8_t hoplimit = metisMessage_GetHopLimit(interestMessage);
 *        if (hoplimit == 0) {
 *           // error
 *        } else {
 *           hoplimit--;
 *        }
 *        // take actions on hoplimit
 *     }
 * }
 * @endcode
 */
bool metisIoOperations_IsLocal(const MetisIoOperations *ops);

/**
 * Returns the connection ID represented by this MetisIoOperations in the ConnectionTable.
 *
 * <#Paragraphs Of Explanation#>
 *
 * @param [in] ops The connection implementation.
 *
 * @return number The connection ID in the connection table.
 *
 * Example:
 * @code
 * {
 *     unsigned id = metisIoOperations_GetConnectionId(ingressIoOps);
 *     const MetisConnection *conn = metisConnectionTable_FindById(metis->connectionTable, id);
 * }
 * @endcode
 */
unsigned metisIoOperations_GetConnectionId(const MetisIoOperations *ops);

/**
 * A pointer that represents the class of the connection
 *
 * Each concrete implementation has a class pointer that is unique to the implementation (not instance).
 * Each implementation is free to choose how to determine the value, so long as it is unique on the system.
 * This is a system-local value.
 *
 * @param [in] ops The connection implementation.
 *
 * @return non-null A pointer value unique to the implementation (not instance).
 *
 * Example:
 * @code
 *   bool
 *   metisEtherConnection_IsInstanceOf(const MetisConnection *conn)
 *   {
 *      bool result = false;
 *      if (conn != NULL) {
 *         MetisIoOperations *ops = metisConnection_GetIoOperations(conn);
 *         const void *class = metisIoOperations_Class(ops);
 *         result = (class == _metisEtherConnection_Class(ops));
 *      }
 *      return result;
 *   }
 * @endcode
 */
const void *metisIoOperations_Class(const MetisIoOperations *ops);

/**
 * Returns the transport type of the connection (TCP, UDP, L2, etc.).
 *
 * TCP and AF_UNIX are both stream connections and will both return "cpiConnection_TCP".
 * Ethernet will return "cpiConnection_L2".
 *
 * @param [in] ops The connection implementation.
 *
 * @return cpiConnection_TCP A TCP4, TCP6, or AF_UNIX connection
 * @return cpiConnection_UDP A UDP4 or UDP6 connection
 * @return cpiConnection_L2 An Ethernet connection
 *
 * Example:
 * @code
 * {
 *     CPIConnectionType type = metisIoOperations_GetConnectionType(metisConnection_GetIoOperations(connection));
 *     CPIConnection *cpiConn = cpiConnection_Create(metisConnection_GetConnectionId(connection), localAddress, remoteAddress, type);
 * }
 * @endcode
 */
CPIConnectionType metisIoOperations_GetConnectionType(const MetisIoOperations *ops);
#endif // Metis_metis_io_h
