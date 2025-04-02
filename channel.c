#include "channel.h"
// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    // Allocate memory for the channel object on the heap
    // The `channel_t` structure will hold the buffer, synchronization primitives, and status information.
    channel_t* new_channel = malloc(sizeof(channel_t));
    if (!new_channel) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Initialize the buffer for the channel
    // The buffer will handle the data storage for the channel with a fixed capacity specified by size.
    buffer_t* buff = buffer_create(size);
    if (!buff) {
        free(new_channel); // Clean up allocated memory if buffer creation fails
        return NULL;
    }

    // Assign the buffer to the channel's buffer field
    new_channel->buffer = buff;

    // Initialize synchronization primitives
    // The mutex (channel_lock) ensures thread-safe operations on the channel.
    // The condition variables (full and empty) are used for signaling between threads.
    pthread_mutex_init(&new_channel->channel_lock, NULL);
    pthread_cond_init(&new_channel->full, NULL);
    pthread_cond_init(&new_channel->empty, NULL);

    // Set the channel status to active
    // The channel_status flag indicates whether the channel is open (true) or closed (false).
    new_channel->channel_status = true;

    // Initialize the sender and receiver lists
    // These lists manage the threads that are currently waiting to send or receive data via the channel.
    new_channel->sel_sends = list_create();
    new_channel->sel_recvs = list_create();

    // Return the newly created channel object
    return new_channel;
}
// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    /* IMPLEMENT THIS */
    // Lock the channel mutex to ensure thread-safe access to the channel's data
    pthread_mutex_lock(&channel->channel_lock);

    // Check if the channel is closed
    // If the channel's status indicates it is closed, return CLOSED_ERROR
    if (!channel->channel_status) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
        return CLOSED_ERROR;
    }

    // Wait for space to become available in the buffer
    // Get the capacity of the buffer
    size_t cap = buffer_capacity(channel->buffer);
    
    // While the buffer is full, wait on the "empty" condition variable
    while (buffer_current_size(channel->buffer) == cap) {
        // Block until the buffer is not full or the channel status changes
        if (pthread_cond_wait(&channel->empty, &channel->channel_lock) != 0) {
            // If an error occurs while waiting, unlock and return a generic error
            pthread_mutex_unlock(&channel->channel_lock);
            return GENERIC_ERROR;
        }

        // Check if the channel has been closed while waiting
        if (!channel->channel_status) {
            pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
            return CLOSED_ERROR;
        }
    }

    // Add the data to the buffer
    // Use the buffer_add() function to insert the data. If it fails, return an error.
    if (buffer_add(channel->buffer, data) == BUFFER_ERROR) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
        return GENERIC_ERROR;
    }

    // Signal the "full" condition variable to notify that data has been added
    pthread_cond_signal(&channel->full);

    // Notify all receivers that are using "select"
    // These are listeners registered for specific channel events
    size_t num_recvs = list_count(channel->sel_recvs); // Get the number of select receivers
    if (num_recvs != 0) {
        list_node_t* head = list_head(channel->sel_recvs); // Get the head of the receiver list

        // Iterate over all select receivers and signal their conditions
        while (head != NULL) {
            // Lock the receiver's mutex
            pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
            
            // Signal the receiver's condition variable to notify about the new data
            pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
            
            // Unlock the receiver's mutex
            pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);

            // Move to the next receiver in the list
            head = head->next;
        }
    }

    // Unlock the channel mutex before returning
    pthread_mutex_unlock(&channel->channel_lock);

    // Return SUCCESS to indicate that the data was successfully written to the channel
    return SUCCESS;
}
// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    // Lock the channel mutex to ensure thread-safe access to the channel's data
    pthread_mutex_lock(&channel->channel_lock);

    // Check if the channel is closed
    // If the channel's status indicates it is closed, return CLOSED_ERROR
    if (!channel->channel_status) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
        return CLOSED_ERROR;
    }

    // Wait for data to become available in the buffer
    // While the buffer is empty, wait on the "full" condition variable
    while (buffer_current_size(channel->buffer) == 0) {
        // Block until the buffer is not empty or the channel status changes
        if (pthread_cond_wait(&channel->full, &channel->channel_lock) != 0) {
            // If an error occurs while waiting, unlock and return a generic error
            pthread_mutex_unlock(&channel->channel_lock);
            return GENERIC_ERROR;
        }

        // Check if the channel has been closed while waiting
        if (!channel->channel_status) {
            pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
            return CLOSED_ERROR;
        }
    }

    // Remove the data from the buffer
    // Use the buffer_remove() function to retrieve the data. If it fails, return an error.
    if (buffer_remove(channel->buffer, data) == BUFFER_ERROR) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
        return GENERIC_ERROR;
    }

    // Signal the "empty" condition variable to notify that space is available
    pthread_cond_signal(&channel->empty);

    // Notify all senders that are using "select"
    // These are senders registered for specific channel events
    size_t num_sends = list_count(channel->sel_sends); // Get the number of select senders
    if (num_sends != 0) {
        list_node_t* head = list_head(channel->sel_sends); // Get the head of the sender list

        // Iterate over all select senders and signal their conditions
        while (head != NULL) {
            // Lock the sender's mutex
            pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
            
            // Signal the sender's condition variable to notify about the available space
            pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
            
            // Unlock the sender's mutex
            pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);

            // Move to the next sender in the list
            head = head->next;
        }
    }

    // Unlock the channel mutex before returning
    pthread_mutex_unlock(&channel->channel_lock);

    // Return SUCCESS to indicate that data was successfully retrieved from the channel
    return SUCCESS;
}
// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    // Acquire the lock to ensure thread-safe access to the channel.
    pthread_mutex_lock(&channel->channel_lock);

    // Check if the channel is closed. If it is, release the lock and return an error status.
    if (!channel->channel_status) {
        pthread_mutex_unlock(&channel->channel_lock);
        return CLOSED_ERROR;
    }

    // Check if the channel's buffer is full.
    size_t cap = buffer_capacity(channel->buffer);
    if (buffer_current_size(channel->buffer) == cap) {
        // If the buffer is full, release the lock and return a full error status.
        pthread_mutex_unlock(&channel->channel_lock);
        return CHANNEL_FULL;
    }

    // Add the data to the buffer.
    // If there is an error during the addition (e.g., memory issue), return a generic error.
    if (buffer_add(channel->buffer, data) == BUFFER_ERROR) {
        pthread_mutex_unlock(&channel->channel_lock);
        return GENERIC_ERROR;
    }

    // Signal any waiting threads that the buffer is no longer empty.
    pthread_cond_signal(&channel->full);

    // Notify all select receivers (threads waiting on this channel for a receive operation).
    size_t num_recvs = list_count(channel->sel_recvs); // Count the number of select receivers.
    if (num_recvs != 0) {
        list_node_t* head = list_head(channel->sel_recvs); // Get the head of the select receivers list.
        while (head != NULL) {
            // Lock each receiver's synchronization structure.
            pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
            // Signal the condition variable associated with the receiver.
            pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
            // Unlock the receiver's synchronization structure.
            pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);
            // Move to the next receiver in the list.
            head = head->next;
        }
    }

    // Release the lock as all operations are complete.
    pthread_mutex_unlock(&channel->channel_lock);

    // Return a success status to indicate the data was sent successfully.
    return SUCCESS;
}
// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    // Acquire the channel lock to ensure thread-safe access to the channel.
    pthread_mutex_lock(&channel->channel_lock);

    // Check if the channel is closed; if so, release the lock and return CLOSED_ERROR.
    if (!channel->channel_status) {
        pthread_mutex_unlock(&channel->channel_lock);
        return CLOSED_ERROR;
    }

    // If the channel's buffer is empty, release the lock and return CHANNEL_EMPTY.
    if (buffer_current_size(channel->buffer) == 0) {
        pthread_mutex_unlock(&channel->channel_lock);
        return CHANNEL_EMPTY;
    }

    // Attempt to remove data from the buffer. If an error occurs, release the lock and return GENERIC_ERROR.
    if (buffer_remove(channel->buffer, data) == BUFFER_ERROR) {
        pthread_mutex_unlock(&channel->channel_lock);
        return GENERIC_ERROR;
    }

    // Signal the condition variable to notify any waiting producers that the buffer is no longer full.
    pthread_cond_signal(&channel->empty);

    // Notify all threads waiting on the `sel_sends` list that they can proceed.
    size_t num_sends = list_count(channel->sel_sends);
    if (num_sends != 0) {
        list_node_t* head = list_head(channel->sel_sends);
        while (head != NULL) {
            // Acquire the lock for the current select sender and signal its condition variable.
            pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
            pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
            pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);
            head = head->next;
        }
    }

    // Release the channel lock after completing all operations.
    pthread_mutex_unlock(&channel->channel_lock);

    // Return SUCCESS to indicate that the data was successfully received.
    return SUCCESS;
}
// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    // Lock the channel mutex to ensure thread-safe access to the channel's state
    pthread_mutex_lock(&channel->channel_lock);

    // Check if the channel is already closed
    if (!channel->channel_status) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock before returning
        return CLOSED_ERROR; // Return an error if the channel is already closed
    }

    // Mark the channel as closed
    channel->channel_status = false;

    // Wake up all threads waiting on the "full" or "empty" condition variables
    // This ensures that senders or receivers blocked on these conditions can terminate
    pthread_cond_broadcast(&channel->full);
    pthread_cond_broadcast(&channel->empty);

    // Notify all select receivers that the channel is now closed
    size_t num_recvs = list_count(channel->sel_recvs); // Get the number of select receivers
    if (num_recvs != 0) {
        list_node_t* head = list_head(channel->sel_recvs); // Get the head of the receiver list

        // Iterate over all select receivers and signal their conditions
        while (head != NULL) {
            sel_sync_t* sel = (sel_sync_t*)head->data; // Cast node data to sel_sync_t
            pthread_mutex_lock(sel->sel_lock);         // Lock the receiver's mutex
            pthread_cond_signal(sel->sel_cond);       // Signal the condition variable
            pthread_mutex_unlock(sel->sel_lock);      // Unlock the receiver's mutex
            head = head->next;                        // Move to the next receiver
        }
    }

    // Notify all select senders that the channel is now closed
    size_t num_sends = list_count(channel->sel_sends); // Get the number of select senders
    if (num_sends != 0) {
        list_node_t* head = list_head(channel->sel_sends); // Get the head of the sender list

        // Iterate over all select senders and signal their conditions
        while (head != NULL) {
            sel_sync_t* sel = (sel_sync_t*)head->data; // Cast node data to sel_sync_t
            pthread_mutex_lock(sel->sel_lock);         // Lock the sender's mutex
            pthread_cond_signal(sel->sel_cond);       // Signal the condition variable
            pthread_mutex_unlock(sel->sel_lock);      // Unlock the sender's mutex
            head = head->next;                        // Move to the next sender
        }
    }

    // Unlock the channel mutex before returning
    pthread_mutex_unlock(&channel->channel_lock);

    // Return SUCCESS to indicate the channel was successfully closed
    return SUCCESS;
}
// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->channel_lock); // Lock the channel mutex to ensure thread-safe access

    // Check if the channel is still open
    if (channel->channel_status == true) {
        pthread_mutex_unlock(&channel->channel_lock); // Unlock the mutex before returning
        return DESTROY_ERROR; // Return an error if the channel is still open
    }

    // Free the channel's buffer
    buffer_free(channel->buffer); // Releases memory allocated for the buffer
    pthread_mutex_unlock(&channel->channel_lock); // Unlock the channel mutex as it's no longer needed

    // Destroy the lists associated with select senders and receivers
    // These lists contain information about threads waiting on channel events
    list_destroy(channel->sel_sends); // Frees memory for the select sender list
    list_destroy(channel->sel_recvs); // Frees memory for the select receiver list

    // Free the channel itself
    free(channel); // Deallocates memory for the channel structure

    // Return SUCCESS to indicate the channel was successfully destroyed
    return SUCCESS;
}
// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /* IMPLEMENT THIS */
    // Initialize local lock and condition variable for synchronization
    pthread_mutex_t local_lock;
    pthread_cond_t local_cond;
    pthread_mutex_init(&local_lock, NULL);
    pthread_cond_init(&local_cond, NULL);

    // Create a synchronization object for select
    sel_sync_t sel_sync;
    sel_sync.sel_lock = &local_lock;
    sel_sync.sel_cond = &local_cond;

    while (true) {
        // Lock all channels to ensure thread-safe checking of conditions
        for (size_t i = 0; i < channel_count; i++) {
            bool dup = false; // Check for duplicate channels in the list
            for (size_t j = 0; j < i; j++) {
                if (channel_list[j].channel == channel_list[i].channel) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                pthread_mutex_lock(&(channel_list[i].channel->channel_lock));
        }

        // Remove any previous synchronization objects from channel queues
        for (size_t i = 0; i < channel_count; i++) {
            if (channel_list[i].dir == SEND) {
                list_node_t* node = list_find(channel_list[i].channel->sel_sends, &sel_sync);
                if (node != NULL) {
                    list_remove(channel_list[i].channel->sel_sends, node);
                }
            } else {
                list_node_t* node = list_find(channel_list[i].channel->sel_recvs, &sel_sync);
                if (node != NULL) {
                    list_remove(channel_list[i].channel->sel_recvs, node);
                }
            }
        }

        // Attempt immediate operations on channels
        for (size_t i = 0; i < channel_count; i++) {
            channel_t* ch = channel_list[i].channel;
            enum direction dir = channel_list[i].dir;

            // Check if the channel is closed
            if (!ch->channel_status) {
                // Unlock all locked channels before returning
                for (size_t k = 0; k < channel_count; k++) {
                    bool dup = false;
                    for (size_t j = 0; j < k; j++) {
                        if (channel_list[j].channel == channel_list[k].channel) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup)
                        pthread_mutex_unlock(&(channel_list[k].channel->channel_lock));
                }
                *selected_index = i;
                return CLOSED_ERROR;
            }

            if (dir == SEND) {
                // Check if the channel buffer has space for sending
                size_t cap = buffer_capacity(ch->buffer);
                if (buffer_current_size(ch->buffer) < cap) {
                    // Add data to buffer
                    if (buffer_add(ch->buffer, channel_list[i].data) == BUFFER_ERROR) {
                        // Unlock all locked channels before returning
                        for (size_t k = 0; k < channel_count; k++) {
                            bool dup = false;
                            for (size_t j = 0; j < k; j++) {
                                if (channel_list[j].channel == channel_list[k].channel) {
                                    dup = true;
                                    break;
                                }
                            }
                            if (!dup)
                                pthread_mutex_unlock(&(channel_list[k].channel->channel_lock));
                        }
                        *selected_index = i;
                        return GENERIC_ERROR;
                    }

                    // Signal any waiting receivers and unlock all channels
                    pthread_cond_signal(&ch->full);
                    list_node_t* head = list_head(ch->sel_recvs);
                    while (head != NULL) {
                        pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
                        pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
                        pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);
                        head = head->next;
                    }

                    *selected_index = i;
                    for (size_t k = 0; k < channel_count; k++) {
                        bool dup = false;
                        for (size_t j = 0; j < k; j++) {
                            if (channel_list[j].channel == channel_list[k].channel) {
                                dup = true;
                                break;
                            }
                        }
                        if (!dup)
                            pthread_mutex_unlock(&(channel_list[k].channel->channel_lock));
                    }
                    return SUCCESS;
                }
            } else { // Receiving case
                // Check if the channel buffer has data for receiving
                if (buffer_current_size(ch->buffer) > 0) {
                    // Remove data from buffer
                    if (buffer_remove(ch->buffer, &channel_list[i].data) == BUFFER_ERROR) {
                        // Unlock all locked channels before returning
                        for (size_t k = 0; k < channel_count; k++) {
                            bool dup = false;
                            for (size_t j = 0; j < k; j++) {
                                if (channel_list[j].channel == channel_list[k].channel) {
                                    dup = true;
                                    break;
                                }
                            }
                            if (!dup)
                                pthread_mutex_unlock(&(channel_list[k].channel->channel_lock));
                        }
                        *selected_index = i;
                        return GENERIC_ERROR;
                    }

                    // Signal any waiting senders and unlock all channels
                    pthread_cond_signal(&ch->empty);
                    list_node_t* head = list_head(ch->sel_sends);
                    while (head != NULL) {
                        pthread_mutex_lock(((sel_sync_t*)head->data)->sel_lock);
                        pthread_cond_signal(((sel_sync_t*)head->data)->sel_cond);
                        pthread_mutex_unlock(((sel_sync_t*)head->data)->sel_lock);
                        head = head->next;
                    }

                    *selected_index = i;
                    for (size_t k = 0; k < channel_count; k++) {
                        bool dup = false;
                        for (size_t j = 0; j < k; j++) {
                            if (channel_list[j].channel == channel_list[k].channel) {
                                dup = true;
                                break;
                            }
                        }
                        if (!dup)
                            pthread_mutex_unlock(&(channel_list[k].channel->channel_lock));
                    }
                    return SUCCESS;
                }
            }
        }

        // If no immediate operation is possible, wait
        pthread_mutex_lock(&local_lock);
        for (size_t i = 0; i < channel_count; i++) {
            bool dup = false;
            for (size_t j = 0; j < i; j++) {
                if (channel_list[j].channel == channel_list[i].channel && channel_list[j].dir == channel_list[i].dir) {
                    dup = true;
                    break;
                }
            }
            if (!dup && channel_list[i].dir == SEND) {
                list_insert(channel_list[i].channel->sel_sends, &sel_sync);
            } else if (!dup && channel_list[i].dir == RECV) {
                list_insert(channel_list[i].channel->sel_recvs, &sel_sync);
            }
        }

        // Unlock all channels before waiting for the condition
        for (size_t i = 0; i < channel_count; i++) {
            bool dup = false;
            for (size_t j = 0; j < i; j++) {
                if (channel_list[j].channel == channel_list[i].channel) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                pthread_mutex_unlock(&(channel_list[i].channel->channel_lock));
        }

        // Wait for a signal to retry
        pthread_cond_wait(&local_cond, &local_lock);
        pthread_mutex_unlock(&local_lock);
    }

    return SUCCESS; // Unreachable, included for completeness
}