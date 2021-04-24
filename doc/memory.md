# Memory Management

This is a design document. None of this is already implemented yet.

## Physical Memory Ownership Model

This section describes how physical memory is shared between the microkernel and
user space. It applies to all hardware architectures, altough details such as
page size, large page size and whether large pages are supported at all are of
course hardware-dependent.

Each page frame (i.e. page of physical memory) is owned by either the
microkernel (kernel page frame) or user space (user page frame). Furthermore,
each page frame is either in use or free:

* For user page frames, being in use means being mapped in one or more process'
address space.
* Any page frame owned by the microkernel is always mapped. Free pages are
linked to a free list until they are allocated. A page frame is in use if it is
allocated.

The microkernel only allocates memory it owns for its own needs and always as
part of the execution of a system call. If allocation fails because insufficient
memory is available, the system call fails with an appropriate error (ENOMEM).

User space (i.e. a user space memory management service) is responsible for
ensuring the microkernel has sufficient memory for its needs. Two system call
are at its disposal to do so:

* One system call allows user space to transfer ownership of page frames to the
microkernel. Only free memory owned by user space can have their ownership
transferred to the microkernel.
* A second system call allows user space to reclaim memory from the microkernel.
Only free memory owned by the microkernel can be reclaimed.

Neither of these two system calls can fail because of a memory allocation
failure caused by insufficient free memory owned by the microkernel.

The microkernel and user space both independently maintain page frame ownership
information. However, the microkernel enforces the following rules in order to
protect itself from user space:

* It will not accept ownership of user page frames that are in use. For this
purpose, the microkernel maintains a per-page frame reference count.
* It will not accept ownership transfer for a page frame it already owns.
* It will restrict how user space is allowed to map page frames it owns. In
general this is disallowed completely. As special cases, some parts of the
memory owned by the microkernel such the boot image are allowed to be mapped
read only by user space.

On some hardware architectures, it might be that only a subset of page frames
can be owned and used by the microkernel.

### Large Page Support

TODO
