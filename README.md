# memoryspeedtest
For testing memory speed and prefetcher characteristics of a given AMD64 windows system

This tool runs multiple timed memory read/writes to check the real practical* read speed in different circumstances.

\* 'real practical' in this sense means as opposed to the speed rating of your system memory.

## Test methodology:
This tool uses a hardcoded (`#define MEMORY_SIZE_GB 10ULL`) amount of memory for it's testing. With exception to the first two tests we only try to read from 1 element of each cache line. When doing this we're counting the entire cache line as being read even though the code only reads 8/64 bytes we treat the full 64bytes as being read. Reading from just 1 element of each cache line removes some of the processors performance from the results so we can focus on how the memory operations work. Since almost every AMD64 system (intel or amd) has a similar cache and mechanism this can prove very reliable. Cache line size of 64bytes is assumed throughout. The test is designed to run on an AMD64 (x64) Windows system. The binary baked in 10GB test memory was chosen as it should allow most users with 16GB of memory to run the tool. Of that 10GB it's devided into elements with a size of 8bytes which is 1 64bit 'word' for an x64 system. The method of 'reading' is varifying that the value in the element matches its index.

When I describe each test I speak in relation to my system and your findings will vary depending on your system, especially when it comes to where your bottlenecks are.

### My System:
```
Intel i7 6700k with an all core overclock to 4.7ghz
Memory DDR4 4x 8GB 3600mhz cl 15-15-15-35 in dual channel configuration. (Theoretical max speed: 57.25GB/s)
Attached is my results in the file result-6700k@4.7ghz-ddr4-3600mhz.txt
```

**Due to the non-realtime nature of Windows even with the process priority set to realtime there will be inconsistencies in the testing and ideally you should close as many applications as possible to improve accuracy of the tests. At the end of the day this program is just reading from a big array of memory, and if your pc can't do that consistently in this synthetic than it can't do it in your normal production workload.**

## The tests:

### Linear read, every byte processed
Literally every element is read in a linear lowest to highest memory address ordering. Due to the 8 reads per cache-line the thread ends up spending a large amount of its time reading within a cache-line for memory bandwidth to be a bottleneck.

### Linear read every second element, every cache line (cpu bottleneck test)
To test how much of a bottleneck is occuring on the processing side of things instead of the memory side of things we skip every second element. Reading 4 times per cache line

### Linear read (offset)
Reading 1 element from the centre of a cacheline as opposed to the start of a cacheline in which the rest of the testing is done.

### Linear read
Reading linear from lowest addressed cacheline to highest.

### Reverse linear read
Reading linear from highest addressed cacheline to lowest.

### Linear read every second cache line
First of the pre-fetcher tests, this test skips every second cacheline, reading just 1 element every second cacheline in accending order.

### Linear read every fourth cache line
Similar to the previous except once every 4th cache line

### Linear read every eighth cache line
Similar to the previous except once every 8th cache line

### Linear read every sixteenth cache line
Similar to the previous except once every 16th cache line

### Reverse linear read every second cache line
Similar to `Linear read every second cache line` except in decending address order

### Multithreaded read, testing cores:
All of the multithreaded tests are done in a similar manor to the `Linear read`. The `Test size n` describes the number of threads used, each thread allocates its own memory and is affinity locked to its own logical processor durring the test. The size of the data that each thread must read is the `MEMORY_SIZE_GB` (Default 10GB) divided by the number of threads e.g.  `Test size 2` each thread tests 1/2 of 10GB.

## AMD64 common cache mechanism explained:
For beginners a great video explainaion is described here: https://www.youtube.com/watch?v=lM-21GySlso

For more detailed description visit wiki which is quite indepth: https://en.wikipedia.org/wiki/CPU_cache

Due to there being ample material already written on CPU cache i'll keep the explanation relatively brief and focused in on why reading 1 element in a 'cache line' is perfectly okay for this test. 
### Cache
Due to the relatively slow speed of system memory almost all computers have at least some form of temporary high speed storage on the cpu itself which is dramatically faster than system memory in combination with having a lot less latency partially because of being so much closer physically. 
### Cache levels
On current modern cpus like intel i7's and AMD ryzens the cpus have 3 levels of cache. L1, L2 and L3 where L3 is usually shared amongst either all or most of the cores. However there are some examples of L4 cache for example the Intel core i7 5775C Broadwell processor which included a huge 128MB l4 cache (https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#Desktop_processors) 

Each level is larger, further away and slower than the last. A side effect of having a bigger cache is it takes longer to read from and is slower. This is one of the main reasons why cpus have had multiple levels of cache since before there were multiple core processors having the need to share cache.

### Cache Lines
Cache lines are typically 64bytes of concurrent data that's cached together. When a cpu thread attempts to access memory it's unable to access the memory directly, it will put a request for the memory in and each level of the cache will be checked for the presents of that memory and if not found finally read a full 64-bytes over multiple memory-reads done by the memory controller into the cache as a single cache line, before handing over that one part of the memory to the thread that needs requested it. Due to this mechanism 

Cache line's are a hardware 'fixed function' feature that comes into the picture as an implimentation artifact of having a cache. I'm going to expand on exactly what I mean by that using an example. On AMD64 a memory pointer is 64bits (8 bytes) that's all that's needed to point to a unique piece of memory. A pointer in Hexidecimal (base16) can be represented like this: 0x12 34 56 78 9A BC DE F0 the '0x' to represent the fact that it's Hexidecimal (or hex for short) and there after each two characters combinds to make 1 byte and there's 8 pairs to make 8 bytes (or 64bits). 

So lets say we have a program that is a clock. The processor recently accessed and needs to access again in the near future the second hand value. 

Lets assume its at the address 0x123456789ABCDEF0 it is just 1 byte that it needs to read the value between 0-59. 
So the CPU could read this value from memory, but that would be slow so we want to cache it to reuse it. So the question is how do we go about caching it? We could store the second hand value somewhere a random number between 0-59, but how will we find it again? The obvious solution is to store the memory address (0x123456789ABCDEF0) and the value (e.g. 0x01) this will allow us to search the cache to check if we have in the cache the value that belongs at the address 0x123456789ABCDEF0. This would work but unfortunetly in this scenario we have to use 8bytes of cache just to store the address of a value of 1Byte. At this rate a 32KByte L1 cache would need to be actually 288KB in size to be able to store 256KB of addresses. This is really wasteful. We could improve on this.

The solution devised is to not just store what we needed to cache but also some of the neighbouring memory, just in case that needs to be accessed as well. There's multiple advantages to this method. So with the before mentioned 64byte cache line size chosen what that means is when we try to cache the one byte at 0x123456789ABCDEF0 the cache will cache 64bytes of 'aligned memory' which will be the range 0x123456789ABCDEC0 to 0x123456789ABCDEFF. At first glance this may appear worse as we're now storing 64bytes AND a 8 byte address just so that we can cache a 1 byte value?!? Lets look at it closer; It's not so bad in the clock example as we likely have minutes at 0x12..DEF1 and hours at 0x12..DEF2 which would both be inadvertently already cached. As an added bonus to the 64byte alignment, it means we don't have to store the low 6 bits of the address because we know it will be between 0 and 63. This brings the address size from 8bytes to 7.25bytes. The address gets smaller still by another optimisation which is organising the cache lines in a predictable manor. In our tiny clock example from yesteryear we might not need the full cacheline but in practice farely large portions are used at once. Example a Player 'class' in a video game might have player health, armor, position, velocity, lets say 5 weapons slots all of which easily adds up to be ~52bytes. In programming there's structured methods of coding called 'virtuals' which have virtual tables that are a series of 8byte pointers which often adds up to at least 64bytes. So often in fact that most modern cpus will 'prefetch' neighbouring memory segments into the cache incase the thread requests for them. Which is one of the features this tool is designed to test.