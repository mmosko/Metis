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
// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Framework.

#pragma weak metisSystem_Interfaces

#include "../metis_System.c"

#include <LongBow/unit-test.h>
#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Memory.h>


// Include the generic tests of MetisSystem
#include "../../test/testrig_metis_System.c"

LONGBOW_TEST_RUNNER(darwin_Interface)
{
    LONGBOW_RUN_TEST_FIXTURE(Global);

    // these are defined in testrig_metis_System.c
    LONGBOW_RUN_TEST_FIXTURE(PublicApi);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(darwin_Interface)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(darwin_Interface)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// ==================================================================

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, metisSystem_Interfaces);
    LONGBOW_RUN_TEST_CASE(Global, metisSystem_InterfaceMTU);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, metisSystem_Interfaces)
{
    MetisForwarder *metis = metisForwarder_Create(NULL);
    CPIInterfaceSet *set = metisSystem_Interfaces(metis);
    assertNotNull(set, "metisSystem_Interfaces return null set");

    // XXX we need some sort of validation test.  e.g. open a socket, then ioctl to
    // XXX get the interface name, then verify its in the list.

    size_t length = cpiInterfaceSet_Length(set);
    assertTrue(length > 0, "metisSystem_Interfaces returned no interfaces");

    for (size_t i = 0; i < length; i++) {
        CPIInterface *iface = cpiInterfaceSet_GetByOrdinalIndex(set, i);
        printf("Interface Index %u\n", cpiInterface_GetInterfaceIndex(iface));
        const CPIAddressList *list = cpiInterface_GetAddresses(iface);
        PARCJSONArray *jsonArray = cpiAddressList_ToJson(list);
        char *str = parcJSONArray_ToString(jsonArray);
        printf("%s\n", str);
        parcMemory_Deallocate((void **) &str);
        parcJSONArray_Release(&jsonArray);
    }

    cpiInterfaceSet_Destroy(&set);
    metisForwarder_Destroy(&metis);
}

// returns a strdup() of the interface name, use free(3)
static char *
_pickInterfaceName(MetisForwarder *metis)
{
    char *ifname = NULL;

    CPIInterfaceSet *set = metisSystem_Interfaces(metis);
    size_t length = cpiInterfaceSet_Length(set);
    assertTrue(length > 0, "metisSystem_Interfaces returned no interfaces");

    for (size_t i = 0; i < length; i++) {
        CPIInterface *iface = cpiInterfaceSet_GetByOrdinalIndex(set, i);
        const CPIAddressList *addressList = cpiInterface_GetAddresses(iface);

        size_t length = cpiAddressList_Length(addressList);
        for (size_t i = 0; i < length && !ifname; i++) {
            const CPIAddress *a = cpiAddressList_GetItem(addressList, i);
            if (cpiAddress_GetType(a) == cpiAddressType_LINK) {
                ifname = strdup(cpiInterface_GetName(iface));
            }
        }
    }

    cpiInterfaceSet_Destroy(&set);
    return ifname;
}

LONGBOW_TEST_CASE(Global, metisSystem_InterfaceMTU)
{
    MetisForwarder *metis = metisForwarder_Create(NULL);

    char *deviceName = _pickInterfaceName(metis);
    unsigned mtu = metisSystem_InterfaceMtu(metis, deviceName);

    assertTrue(mtu > 0, "Did not get mtu for interface %s", deviceName);
    free(deviceName);
    metisForwarder_Destroy(&metis);
}

// ==================================================================

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(darwin_Interface);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
