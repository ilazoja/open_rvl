#include "OSMessage.h"
#include "OSInterrupt.h"

void OSInitMessageQueue(OSMessageQueue* queue, OSMessage* buffer,
                        s32 capacity) {
    OSInitThreadQueue(&queue->sendQueue);
    OSInitThreadQueue(&queue->recvQueue);
    queue->buffer = buffer;
    queue->capacity = capacity;
    queue->front = 0;
    queue->size = 0;
}

BOOL OSSendMessage(OSMessageQueue* queue, OSMessage mesg, u32 flags) {
    s32 mesgId;
    u32 interrupt;

    interrupt = OSDisableInterrupts();

    while (queue->capacity <= queue->size) {
        if (!(flags & OS_MSG_PERSISTENT)) {
            OSRestoreInterrupts(interrupt);
            return FALSE;
        }

        OSSleepThread(&queue->sendQueue);
    }

    mesgId = (queue->front + queue->size) % queue->capacity;
    queue->buffer[mesgId] = mesg;
    queue->size++;

    OSWakeupThread(&queue->recvQueue);
    OSRestoreInterrupts(interrupt);
    return TRUE;
}

BOOL OSReceiveMessage(OSMessageQueue* queue, OSMessage* mesg, u32 flags) {
    u32 interrupt;

    interrupt = OSDisableInterrupts();

    while (queue->size == 0) {
        if (!(flags & OS_MSG_PERSISTENT)) {
            OSRestoreInterrupts(interrupt);
            return FALSE;
        }

        OSSleepThread(&queue->recvQueue);
    }

    if (mesg != NULL) {
        *mesg = queue->buffer[queue->front];
    }
    queue->front++;
    queue->front %= queue->capacity;
    queue->size--;

    OSWakeupThread(&queue->sendQueue);
    OSRestoreInterrupts(interrupt);
    return TRUE;
}

BOOL OSJamMessage(OSMessageQueue* queue, OSMessage mesg, u32 flags) {
    s32 lastMesg;
    u32 interrupt;

    interrupt = OSDisableInterrupts();

    while (queue->capacity <= queue->size) {
        if (!(flags & OS_MSG_PERSISTENT)) {
            OSRestoreInterrupts(interrupt);
            return FALSE;
        }

        OSSleepThread(&queue->sendQueue);
    }

    // Find last position in queue
    lastMesg = (queue->front + queue->capacity - 1) % queue->capacity;
    queue->front = lastMesg;
    queue->buffer[lastMesg] = mesg;
    queue->size++;

    OSWakeupThread(&queue->recvQueue);
    OSRestoreInterrupts(interrupt);
    return TRUE;
}
