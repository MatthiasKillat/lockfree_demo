# Lock Free Demo

Contains code presented at Meeting C++ 2021

Lock-free Programming for Real-Time Systems
How Compare Exchange Will Become Your New Best Friend
## General

Showcase basic usage of compare_exchange in C++ (CAS loop)

- implement fetch_multiply similarly to fetch_add

There is also non-lockfree code to showcase the pitfalls.

## ExchangeBuffer

- lock-free structure to exchange data between threads
- may contain only one data element
- allows writing, reading and taking (read and remove from buffer) data
- uses (simple) lock-free memory management
- not  completely lock-free due to std::optional (can be replaced)

## Lockfree Memory Management
### Lock-free Storage
Simple object pool for objects of type T.

### Lock-free IndexPool
Management of access to slots in the Storage.

Together with Storage this leads to a simple lock-free allocator.

## SyncCounter

Artificial example on  how to update two memory locations in a consistent way.
Consider two counters and each operation must make sure that it keeps the counters synchronized before proceeding.
We call these states of equal counters consistent.

The operations are incrementing and reading the counter (something more interesting is required here).
We may have some partial updates in this case, meaning one counter was updated but the other was not yet updated.

We do not allow these counters to differ by more than 1 at any time. To achieve this a helper paradigm is used.

This all would not be needed if we would have Double CAS to update two locations atomically at once (this operation does not exist on practical hardware).

### Helper Paradigm

Each operation checks whether the object is in a consistent states, i.e. the counters are equal.
If not some partial update is in progress and it *helps* to complete
this update to obtain a consistent state. The operation proceeds normally after helping a partially completed operation.
The tricky part is ensuring that each operation (increment) only completes once, either on its won or with help.

This paradigm is used to seemingly simultaneously update a write position and a written value in e.g. some queue implementations.

## Tests

- unit tests for basic funtionality of the lock-free ExchangeBuffer
- basic stress tests for the lock-free ExchangeBuffer
- simple stress test for the SyncCounter
- tests would need to be extended for production use

## Further references

More lock-free and concurrrent code can be found in

https://github.com/eclipse-iceoryx/iceoryx

in particular in

https://github.com/eclipse-iceoryx/iceoryx/tree/master/iceoryx_hoofs/include/iceoryx_hoofs/internal/concurrent

there are highly-specialized lock-free queue implementations.They do not use exceptions nor dynamic memory in any way, but this also leads to restrictions.

We can also find an alternative to std::optional in https://github.com/eclipse-iceoryx/iceoryx/blob/master/iceoryx_hoofs/include/iceoryx_hoofs/cxx/optional.hpp which does not use dynamic memory. This can be used to arrive at a truly lock-free implementation of the ExchangeBUffer.
