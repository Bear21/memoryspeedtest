// memoryspeedtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "timecounter.h"

#include <stdio.h>
#include <thread>

#define MEMORY_ELEMENT_COUNT (1024ULL * 1024ULL * 1024ULL * 16ULL / 8ULL)
volatile int Nthread = 0;

void SetupThreadPriority()
{
   HANDLE thr = GetCurrentThread();
   SetThreadPriority(thr, THREAD_PRIORITY_TIME_CRITICAL);
}

void SetAffinity(HANDLE thread, int affinity)
{
   SetThreadAffinityMask(thread, 1 << affinity);
}

void Prepare(HANDLE thread, int affinity)
{
   SetAffinity(thread, affinity);
   SetupThreadPriority();
}

float ThreadWork(int affinity, volatile LONG* lock)
{
   {
      HANDLE thr = GetCurrentThread();
      Prepare(thr, affinity);
   }

   __int64* memory = new __int64[MEMORY_ELEMENT_COUNT / Nthread];

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT / Nthread; i++)
   {
      memory[i] = i;
   }
   TimePast tp;
   InterlockedAdd(lock, -1);

   while (*lock != 0);
   tp.Reset();
   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT / Nthread; i+=1)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
      }
   }
   float ret = tp.Peek();
   delete [] memory;

   InterlockedAdd(lock, 1);
   while (*lock != Nthread);
   return ret;
}

void ThreadTest()
{
   volatile LONG lock = 0;

   DWORD_PTR dwSystemAffinity, dwProcessAffinity;
   if (GetProcessAffinityMask(GetCurrentProcess(),
      &dwProcessAffinity,
      &dwSystemAffinity))
   {
      // Get the number of logical processors on the system.
      SYSTEM_INFO si = { 0 };
      GetSystemInfo(&si);

      // Get the number of logical processors available to the process.
      DWORD dwAvailableLogProcs = 0;
      for (DWORD_PTR lp = 1; lp; lp <<= 1)
      {
         if (dwProcessAffinity & lp)
            ++dwAvailableLogProcs;
      }
      Nthread = (char)dwAvailableLogProcs;
   }
   lock = Nthread;
   std::thread** thrs = new (std::thread*[Nthread]);
   for (int i = 1; i < Nthread; i++) {
      thrs[i] = new std::thread(ThreadWork, i, &lock);
   }
   float time = ThreadWork(0, &lock);
   printf("Verification took %0.2fsec\n", time);
   for (int i = 1; i < Nthread; i++) {
      if (thrs[i]->joinable()) {
         thrs[i]->join();
      }
      delete thrs[i];
   }
   delete thrs;
}

int main()
{
   SetupThreadPriority();
   __int64* memory = new __int64[MEMORY_ELEMENT_COUNT];

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i++)
   {
      memory[i] = i;
   }
   TimePast tp;

   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i++)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   float time = tp.Check();
   printf("Verification took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i+=8)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification cache bounds took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = MEMORY_ELEMENT_COUNT-1; i >= 0; i -= 8)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification cache bounds reverse took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i += 2)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification 1/2 elements took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i += 16)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification every Second cache line took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i += 32)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification every Fourth cache line took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i += 64)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification every Eighth cache line took %0.2fsec\n", time);
   tp.Reset();

   for (__int64 i = MEMORY_ELEMENT_COUNT - 1; i >= 0; i -= 16)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   printf("Verification every Second cache line in reverse took %0.2fsec\n", time);
   tp.Reset();

   delete memory;

   ThreadTest();
   ThreadTest();
   ThreadTest();

   system("PAUSE");
   return 0;
}

