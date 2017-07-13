#ifndef _JINUE_ASM_IPC_H
#define _JINUE_ASM_IPC_H

/*  +-----------------------+------------------------+---------------+
    |      buffer_size      |       data_size        |     n_desc    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0 */

/** number of bits reserved for the message buffer size and data size fields */
#define JINUE_SEND_SIZE_BITS            12

/** number of bits reserved for the number of message descriptors */
#define JINUE_SEND_N_DESC_BITS          8

/** maximum size of a message buffer and of the data inside that buffer */
#define JINUE_SEND_MAX_SIZE             (1 << (JINUE_SEND_SIZE_BITS - 1))

/** maximum number of descriptors inside a message */
#define JINUE_SEND_MAX_N_DESC           ((1 << JINUE_SEND_N_DESC_BITS) - 1)

/** mask to extract the message buffer or data size fields */
#define JINUE_SEND_SIZE_MASK            ((1 << JINUE_SEND_SIZE_BITS) - 1)

/** mask to extract the number of descriptors inside a message */
#define JINUE_SEND_N_DESC_MASK          JINUE_SEND_MAX_N_DESC

/** offset of buffer size within arg3 */
#define JINUE_SEND_BUFFER_SIZE_OFFSET   (JINUE_SEND_N_DESC_BITS + JINUE_SEND_SIZE_BITS)

/** offset of data size within arg3 */
#define JINUE_SEND_DATA_SIZE_OFFSET     JINUE_SEND_N_DESC_BITS

/** offset of number of descriptors within arg3 */
#define JINUE_SEND_N_DESC_OFFSET        0

#endif
