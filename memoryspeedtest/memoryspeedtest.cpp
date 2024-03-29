// memoryspeedtest.cpp : Defines the entry point for the console application.
// For testing memory performance on AMD64 systems running under windows OS
//
//  Copyright(C) 2019  Stephen Wheeler - 8bitbear.com - https://github.com/Bear21
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see<https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "timecounter.h"

#include <stdio.h>
#include <thread>

#define MEMORY_SIZE_GB 10ULL
#define MEMORY_SIZE_GB_FLOAT ((float)MEMORY_SIZE_GB)
#define MEMORY_ELEMENT_SIZE 8ULL
#define MEMORY_ELEMENT_COUNT (1024ULL * 1024ULL * 1024ULL * MEMORY_SIZE_GB / MEMORY_ELEMENT_SIZE)

volatile int Nthread = 0;
volatile int testSize = 0;

void PrintResult(const char* test, float time, float amount)
{
   printf("%s\nVerification took %0.2fsec (%.2fGB/s)\n\n", test, time, (float)amount/time);
}

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
   size_t elementCount = MEMORY_ELEMENT_COUNT / testSize;

   __int64* memory = new __int64[elementCount];

   for (__int64 i = 0; i < elementCount; i++)
   {
      memory[i] = i;
   }
   TimePast tp;
   InterlockedAdd(lock, -1);

   while (*lock != 0);
   tp.Reset();
   for (__int64 i = 0; i < elementCount; i+=8)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
      }
   }
   float ret = tp.Peek();
   delete [] memory;

   InterlockedAdd(lock, 1);
   while (*lock != testSize);
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
   for (int j = 1; j <= Nthread; j*=2) {
      lock = Nthread;
      std::thread** thrs = new (std::thread*[Nthread]);
      printf("Multithreaded read, testing cores: 0");
      int counter = 1;
      
      for (int i = j; i < Nthread; i += j) {
         counter++;
      }
      lock = counter;
      testSize = lock;
      counter = 0;
      for (int i = j; i < Nthread; i+=j) {
         
         thrs[counter++] = new std::thread(ThreadWork, i, &lock);
         printf(", %d", i);
      }
      puts("");
      float time = ThreadWork(0, &lock);
      printf("Test size %d\n", testSize);
      PrintResult("Multithreaded seperated linear read", time, MEMORY_SIZE_GB_FLOAT);
      for (int i = 0; i < counter; i++) {
         if (thrs[i]->joinable()) {
            thrs[i]->join();
         }
         delete thrs[i];
      }
      delete thrs;
   }
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
   PrintResult("Linear read, every byte processed", time, MEMORY_SIZE_GB_FLOAT);
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
   PrintResult("Linear read every second element, every cache line (cpu bottleneck test)", time, MEMORY_SIZE_GB_FLOAT);
   tp.Reset();

   for (__int64 i = 4; i < MEMORY_ELEMENT_COUNT; i += 8)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   PrintResult("Linear read (offset)", time, MEMORY_SIZE_GB_FLOAT);
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
   PrintResult("Linear read", time, MEMORY_SIZE_GB_FLOAT);
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
   PrintResult("Reverse linear read", time, MEMORY_SIZE_GB_FLOAT);
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
   PrintResult("Linear read every second cache line", time, MEMORY_SIZE_GB_FLOAT/2.f);
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
   PrintResult("Linear read every fourth cache line", time, MEMORY_SIZE_GB_FLOAT/4.f);
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
   PrintResult("Linear read every eighth cache line", time, MEMORY_SIZE_GB_FLOAT/8.f);
   tp.Reset();

   for (__int64 i = 0; i < MEMORY_ELEMENT_COUNT; i += 128)
   {
      if (memory[i] != i) {
         printf("Failed");
         system("PAUSE");
         return 0;
      }
   }
   time = tp.Check();
   PrintResult("Linear read every sixteenth cache line", time, MEMORY_SIZE_GB_FLOAT/16.f);
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
   PrintResult("Reverse linear read every second cache line", time, MEMORY_SIZE_GB_FLOAT/2.f);
   tp.Reset();

   delete memory;

   ThreadTest();
   ThreadTest();
   ThreadTest();

   system("PAUSE");
   return 0;
}

