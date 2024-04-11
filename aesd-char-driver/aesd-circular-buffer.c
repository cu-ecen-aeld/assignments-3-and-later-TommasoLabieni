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
#include <stdio.h>
void print_buffer(struct aesd_circular_buffer *buffer)
{
    for (uint8_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        printf("[%u]: %s\n", i, buffer->entry[i].buffptr);
    }
}

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
                                                                          size_t char_offset, size_t *entry_offset_byte_rtn)
{
    uint8_t idx = buffer->out_offs;
    printf("------ BUFFER ------\n");
    print_buffer(buffer);
    printf("------ END BUFFER ------\n");

    if (char_offset == 0)
    {
        printf("char off is zero.\n");
        *entry_offset_byte_rtn = 0;
        return &(buffer->entry[idx]);
    }

    while (true)
    {
        printf("Curr char_offset: %lu\n", char_offset);
        printf("String is: %s Len is: %lu\n", buffer->entry[idx].buffptr, buffer->entry[idx].size);
        if (buffer->entry[idx].size <= char_offset)
        {
            char_offset -= buffer->entry[idx].size;
            idx += 1;
            idx %= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
            if (buffer->in_offs == idx)
                return NULL;
        }
        else
        {
            /**
             * If here, this is the correct idx of buffer->entry.
             */
            *entry_offset_byte_rtn = char_offset;
            return &(buffer->entry[idx]);
        }
    }
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
    /**
     * Add element to the buffer
     */

    printf("Written: %s\n", add_entry->buffptr);
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs += 1;
    buffer->in_offs %= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    printf("IN OFFS: %d - OUT OFFS: %d\n", buffer->in_offs, buffer->out_offs);

    /**
     * Check if buffer is full
     */
    if (buffer->in_offs == buffer->out_offs)
    {
        printf("BUFFER IS FULL!\n");
        buffer->full = true;
        return;
    }

    if (buffer->full)
    {
        printf("Unset\n");
        buffer->full = false;

        buffer->out_offs += 1;
        buffer->out_offs %= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        printf("NEW:::IN OFFS: %d - OUT OFFS: %d\n", buffer->in_offs, buffer->out_offs);
    }
}

/**
 * Initializes the circular buffer described by @param buffer to an empty struct
 */
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
}