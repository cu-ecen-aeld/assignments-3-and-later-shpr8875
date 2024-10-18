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
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
     if (buffer == NULL || entry_offset_byte_rtn == NULL) 
     {
        return NULL;
     }

    size_t buffer_size = 0;
    uint8_t index = buffer->out_offs;
    struct aesd_buffer_entry *buf_entry;

    // Loop until wrap around to the start
    while (1) 
    {
        buf_entry = &buffer->entry[index];

        if (buf_entry->buffptr != NULL) 
        {
            if (char_offset >= buffer_size && char_offset < (buffer_size + buf_entry->size)) 
            {
                *entry_offset_byte_rtn = char_offset - buffer_size;
                return buf_entry;  // Found the entry
            }
            buffer_size += buf_entry->size;
        }

        // Increment index and wrap around if necessary
        index++;
        
        // CHeck for max buffer operation
        if (index >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
        {
            index = 0;
        }

        // Break the loop after wrap around
        if (index == buffer->out_offs) 
        {
            break;
        }
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
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    // Null pointer checks
    if (buffer == NULL || add_entry == NULL) 
    {
        return;
    }
       
     // adding elements in buffer
    buffer->entry[buffer->in_offs] = *add_entry;
    
    // Move up in_offs
    buffer->in_offs++;
    
    if (buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {
        buffer->in_offs = 0;
    }

    // Check if the buffer is full
    if (buffer->full) 
    {
        // If full, overwrite the oldest entry
        buffer->out_offs = buffer->in_offs;
    }

    // Update full status
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
