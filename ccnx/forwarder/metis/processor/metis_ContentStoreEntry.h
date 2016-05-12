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

//
//  metis_ContentStoreEntry.h
//  Metis
//
//  Created by Mosko, Marc <Marc.Mosko@parc.com> on 11/29/13.

#ifndef Metis_metis_ContentStoreEntry_h
#define Metis_metis_ContentStoreEntry_h

#include <ccnx/forwarder/metis/core/metis_Message.h>
#include <ccnx/forwarder/metis/processor/metis_LruList.h>

struct metis_content_store_entry;
typedef struct metis_content_store_entry MetisContentStoreEntry;

/**
 * @function metisContentStoreEntry_Create
 * @abstract Creates a content store entry, saving a reference to the message
 * @discussion
 *   When <code>metisContentStoreEntry_Destroy()</code> is called, will release its reference
 *
 * @param <#param1#>
 * @return <#return#>
 */
MetisContentStoreEntry *metisContentStoreEntry_Create(MetisMessage *objectMessage, MetisLruList *lruList);

/**
 * @function metisContentStoreEntry_Copy
 * @abstract Returns a reference counted copy
 * @discussion
 *   <#Discussion#>
 *
 * @param <#param1#>
 * @return Reference counted copy, must call <code>metisContentStoreEntry_Destroy()</code> on it.
 */
MetisContentStoreEntry *metisContentStoreEntry_Acquire(MetisContentStoreEntry *original);

/**
 * Releases one reference count and destroys object when reaches zero
 *
 * <#Paragraphs Of Explanation#>
 *
 * @param [in,out] entryPtr A pointer to an allocated MetisContentStoreEntry
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
void metisContentStoreEntry_Release(MetisContentStoreEntry **entryPtr);

/**
 * @function metisContentStoreEntry_GetMessage
 * @abstract Returns a reference counted copy of the message.
 * @discussion
 *   Caller must use <code>metisMessage_Release()</code> on the returned message
 *
 * @param <#param1#>
 * @return <#return#>
 */
MetisMessage *metisContentStoreEntry_GetMessage(MetisContentStoreEntry *storeEntry);

/**
 * Move this entry to the head of the LRU list
 *
 * Moves the entry to the head of the LRU list it was created with
 *
 * @param [in] storeEntry An allocated MetisContenstoreEntry
 *
 * Example:
 * @code
 * <#example#>
 * @endcode
 */
void metisContentStoreEntry_MoveToHead(MetisContentStoreEntry *storeEntry);
#endif // Metis_metis_ContentStoreEntry_h
