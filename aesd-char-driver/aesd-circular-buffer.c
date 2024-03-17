/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos ( struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t totalBytes = 0;
    uint8_t out = buffer->out_offs;
    uint8_t i;
    // Check for bad buffer or offset input
    if (  ( buffer == NULL )
       || ( entry_offset_byte_rtn == NULL ) )
    {
        return NULL;
    }

    for ( i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++ )
    {
        // Check if the current entry is empty
        if ( buffer->entry[out].buffptr == NULL )
        {
            break;
        }

        // Check if entery contains char offset
        if (totalBytes + buffer->entry[out].size > char_offset) {
            // Calc offset within entry
            *entry_offset_byte_rtn = char_offset - totalBytes;
            return &buffer->entry[out];
        }
        // Increase total bytes
        totalBytes += buffer->entry[out].size;
        // Move to the next entry
        out = ( out + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char *aesd_circular_buffer_add_entry ( struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry )
{
    const char* entryPtr = NULL;
    uint8_t *inOffset = &buffer->in_offs;
    uint8_t *outOffset = &buffer->out_offs;

    // Check for bad buffer or entry input
    if (  ( buffer == NULL )
       || ( add_entry == NULL ) )
    {
        return entryPtr;
    }

    // If full, increment out offset
    if ( buffer->full )
    {
        *outOffset = ( *outOffset + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entryPtr = buffer->entry[*inOffset].buffptr;
    }

    // Add the entry to latest position
    buffer->entry[*inOffset] = *add_entry;

    // Increment in offset
    *inOffset = ( *inOffset + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // Set full if in and out are equal
    buffer->full = ( *inOffset == *outOffset );
    return entryPtr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init ( struct aesd_circular_buffer *buffer )
{
    memset( buffer, 0, sizeof( struct aesd_circular_buffer ) );
}
