// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    osdcore.h

    Core OS-dependent code interface.

****************************************************************************

    The prototypes in this file describe the interfaces that the MAME core
    and various tools rely upon to interact with the outside world. They are
    broken out into several categories.

***************************************************************************/

#pragma once
#include "osdcomm.h"

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/***************************************************************************
    TIMING INTERFACES
***************************************************************************/

/* a osd_ticks_t is a 64-bit unsigned integer that is used as a core type in timing interfaces */
typedef uint64_t osd_ticks_t;

/*-----------------------------------------------------------------------------
    osd_ticks: return the current running tick counter

    Parameters:

        None

    Return value:

        an osd_ticks_t value which represents the current tick counter

    Notes:

        The resolution of this counter should be 1ms or better for accurate
        performance. It is also important that the source of this timer be
        accurate. It is ok if this call is not ultra-fast, since it is
        primarily used for once/frame synchronization.
-----------------------------------------------------------------------------*/
osd_ticks_t osd_ticks(void);

/*-----------------------------------------------------------------------------
    osd_ticks_per_second: return the number of ticks per second

    Parameters:

        None

    Return value:

        an osd_ticks_t value which represents the number of ticks per
        second
-----------------------------------------------------------------------------*/
osd_ticks_t osd_ticks_per_second(void);

/*-----------------------------------------------------------------------------
    osd_sleep: sleep for the specified time interval

    Parameters:

        duration - an osd_ticks_t value that specifies how long we should
            sleep

    Return value:

        None

    Notes:

        The OSD layer should try to sleep for as close to the specified
        duration as possible, or less. This is used as a mechanism to
        "give back" unneeded time to other programs running in the system.
        On a simple, non multitasking system, this routine can be a no-op.
        If there is significant volatility in the amount of time that the
        sleep occurs for, the OSD layer should strive to sleep for less time
        than specified rather than sleeping too long.
-----------------------------------------------------------------------------*/
void osd_sleep(osd_ticks_t duration);

/***************************************************************************
    WORK ITEM INTERFACES
***************************************************************************/

/* this is the maximum number of supported threads for a single work queue */
/* threadid values are expected to range from 0..WORK_MAX_THREADS-1 */
#define WORK_MAX_THREADS 16

/* these flags can be set when creating a queue to give hints to the code about
   how to configure the queue */
#define WORK_QUEUE_FLAG_IO 0x0001
#define WORK_QUEUE_FLAG_MULTI 0x0002
#define WORK_QUEUE_FLAG_HIGH_FREQ 0x0004

/* these flags can be set when queueing a work item to indicate how to handle
   its deconstruction */
#define WORK_ITEM_FLAG_AUTO_RELEASE 0x0001

/* osd_work_queue is an opaque type which represents a queue of work items */
struct osd_work_queue;

/* osd_work_item is an opaque type which represents a single work item */
struct osd_work_item;

/* osd_work_callback is a callback function that does work */
typedef void* (*osd_work_callback)(void* param, int threadid);

/*-----------------------------------------------------------------------------
    osd_work_queue_alloc: create a new work queue

    Parameters:

        flags - one or more of the WORK_QUEUE_FLAG_* values ORed together:

            WORK_QUEUE_FLAG_IO - indicates that the work queue will do some
                I/O; this may be a useful hint so that threads are created
                even on single-processor systems since I/O can often be
                overlapped with other work

            WORK_QUEUE_FLAG_MULTI - indicates that the work queue should
                take advantage of as many processors as it can; items queued
                here are assumed to be fully independent or shared

            WORK_QUEUE_FLAG_HIGH_FREQ - indicates that items are expected
                to be queued at high frequency and acted upon quickly; in
                general, this implies doing some spin-waiting internally
                before falling back to OS-specific synchronization

    Return value:

        A pointer to an allocated osd_work_queue object.

    Notes:

        A work queue abstracts the notion of how potentially threaded work
        can be performed. If no threading support is available, it is a
        simple matter to execute the work items as they are queued.
-----------------------------------------------------------------------------*/
osd_work_queue* osd_work_queue_alloc(int flags);

/*-----------------------------------------------------------------------------
    osd_work_queue_items: return the number of pending items in the queue

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

    Return value:

        The number of incomplete items remaining in the queue.
-----------------------------------------------------------------------------*/
int osd_work_queue_items(osd_work_queue* queue);

/*-----------------------------------------------------------------------------
    osd_work_queue_wait: wait for the queue to be empty

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

        timeout - a timeout value in osd_ticks_per_second()

    Return value:

        true if the queue is empty; false if the wait timed out before the
        queue was emptied.
-----------------------------------------------------------------------------*/
bool osd_work_queue_wait(osd_work_queue* queue, osd_ticks_t timeout);

/*-----------------------------------------------------------------------------
    osd_work_queue_free: free a work queue, waiting for all items to complete

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_work_queue_free(osd_work_queue* queue);

/*-----------------------------------------------------------------------------
    osd_work_item_queue_multiple: queue a set of work items

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

        callback - pointer to a function that will do the work

        numitems - number of work items to queue

        param - a void * parameter that can be used to pass data to the
            function

        paramstep - the number of bytes to increment param by for each item
            queued; for example, if you have an array of work_unit objects,
            you can point param to the base of the array and set paramstep to
            sizeof(work_unit)

        flags - one or more of the WORK_ITEM_FLAG_* values ORed together:

            WORK_ITEM_FLAG_AUTO_RELEASE - indicates that the work item
                should be automatically freed when it is complete

    Return value:

        A pointer to the final allocated osd_work_item in the list.

    Notes:

        On single-threaded systems, this function may actually execute the
        work item immediately before returning.
-----------------------------------------------------------------------------*/
osd_work_item* osd_work_item_queue_multiple(osd_work_queue* queue, osd_work_callback callback, int32_t numitems,
                                            void* parambase, int32_t paramstep, uint32_t flags);

/* inline helper to queue a single work item using the same interface */
static inline osd_work_item* osd_work_item_queue(osd_work_queue* queue, osd_work_callback callback, void* param,
                                                 uint32_t flags)
{
  return osd_work_item_queue_multiple(queue, callback, 1, param, 0, flags);
}

/*-----------------------------------------------------------------------------
    osd_work_item_wait: wait for a work item to complete

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

        timeout - a timeout value in osd_ticks_per_second()

    Return value:

        true if the item completed; false if the wait timed out before the
        item completed.
-----------------------------------------------------------------------------*/
bool osd_work_item_wait(osd_work_item* item, osd_ticks_t timeout);

/*-----------------------------------------------------------------------------
    osd_work_item_result: get the result of a work item

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

    Return value:

        A void * that represents the work item's result.
-----------------------------------------------------------------------------*/
void* osd_work_item_result(osd_work_item* item);

/*-----------------------------------------------------------------------------
    osd_work_item_release: release the memory allocated to a work item

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

    Return value:

        None.

    Notes:

        The osd_work_item exists until explicitly released, even if it has
        long since completed. It is the queuer's responsibility to release
        any work items it has queued.
-----------------------------------------------------------------------------*/
void osd_work_item_release(osd_work_item* item);
