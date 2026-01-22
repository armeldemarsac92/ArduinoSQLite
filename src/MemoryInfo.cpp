#include "MemoryInfo.hpp"

#include <smalloc.h>

// note: these values are defined by the linker, they are not valid memory
// locations in all cases - by defining them as arrays, the C++ compiler
// will use the address of these definitions - it's a big hack, but there's
// really no clean way to get at linker-defined symbols from the .ld file
extern char _stext[], _etext[], _sbss[], _ebss[], _sdata[], _edata[], _estack[], _heap_start[], _heap_end[], _itcm_block_count[], * __brkval;

#ifdef ARDUINO_TEENSY41
extern char _extram_start[], _extram_end[];
extern "C" uint8_t external_psram_size;
#endif

namespace halvoe::memoryInfo
{
  // ---- memory infos about various memory regions ----
  // based on https://forum.pjrc.com/index.php?threads/memory-usage-teensy-4-1.72235/ 
  
  constexpr uint32_t getRamStart()
  {
    return 0x2020'0000;
  }

  constexpr uint32_t getRamSize()
  {
    return 512 << 10;
  }

  constexpr uint32_t getRamEnd()
  {
    return getRamStart() + getRamSize() - 1;
  }

  constexpr uint32_t getFlashStart()
  {
    return 0x6000'0000;
  }

  constexpr uint32_t getFlashSize()
  {
#ifdef ARDUINO_TEENSY40
    return 2 << 20;
#elif ARDUINO_TEENSY41
    return 8 << 20;
#endif
  }

  constexpr uint32_t getFlashEnd()
  {
    return getFlashStart() + getFlashSize() - 1;
  }

  uint32_t getCodeStart()
  {
    return reinterpret_cast<uint32_t>(_stext);
  }

  uint32_t getCodeEnd()
  {
    return reinterpret_cast<uint32_t>(_etext);
  }

  uint32_t getCodeInBytes()
  {
    return getCodeEnd() - getCodeStart();
  }

  uint32_t getInitialisedDataStart()
  {
    return reinterpret_cast<uint32_t>(_sdata);
  }

  uint32_t getInitialisedDataEnd()
  {
    return reinterpret_cast<uint32_t>(_edata);
  }

  uint32_t getInitialisedDataInBytes()
  {
    return getInitialisedDataEnd() - getInitialisedDataStart();
  }

  uint32_t getUninitialisedDataStart()
  {
    return reinterpret_cast<uint32_t>(_sbss);
  }

  uint32_t getUninitialisedDataEnd()
  {
    return reinterpret_cast<uint32_t>(_ebss);
  }

  uint32_t getUninitialisedDataInBytes()
  {
    return getUninitialisedDataEnd() - getUninitialisedDataStart();
  }

  uint32_t getStackPointer()
  {
    return reinterpret_cast<uint32_t>(__builtin_frame_address(0));
  }

  uint32_t getAvailableStackInBytes()
  {
    return getStackPointer() - getUninitialisedDataEnd();
  }

  uint32_t getStackEnd()
  {
    return reinterpret_cast<uint32_t>(_estack);
  }

  uint32_t getUsedStackInBytes()
  {
    return getStackEnd() - getStackPointer();
  }

  uint32_t getHeapPointer()
  {
    return reinterpret_cast<uint32_t>(__brkval);
  }

  uint32_t getHeapStart()
  {
    return reinterpret_cast<uint32_t>(_heap_start);
  }

  uint32_t getHeapEnd()
  {
    return reinterpret_cast<uint32_t>(_heap_end);
  }

  uint32_t getAvailableHeapInBytes()
  {
    return getHeapEnd() - getHeapPointer();
  }

  uint32_t getUsedHeapInBytes()
  {
    return getHeapPointer() - getHeapStart();
  }

  uint32_t getItcmStart()
  {
    return getCodeStart();
  }

  uint32_t getItcmEnd()
  {
    return getItcmStart() + (reinterpret_cast<uint32_t>(_itcm_block_count) << 15) - 1;
  }

  uint32_t getDtcmStart()
  {
    return getInitialisedDataStart();
  }

  uint32_t getDtcmEnd()
  {
    return getStackEnd() - 1;
  }

#if ARDUINO_TEENSY41
  uint32_t getStaticPsramPointer()
  {
    return reinterpret_cast<uint32_t>(_extram_end);
  }

  uint32_t getStaticPsramStart()
  {
    return reinterpret_cast<uint32_t>(_extram_start);
  }

  uint32_t getStaticPsramEnd()
  {
    return external_psram_size > 0 ? (getStaticPsramStart() + (external_psram_size << 20) - 1) : 0;
  }

  uint32_t getStaticAvailablePsramInBytes()
  {
    return external_psram_size > 0 ? (getStaticPsramStart() + (external_psram_size << 20) - getStaticPsramPointer()) : 0;
  }

  uint32_t getStaticUsedPsramInBytes()
  {
    return getStaticPsramPointer() - getStaticPsramStart();
  }

  DynamicPSRAMInfo getDynamicPsramInfo()
  {
    DynamicPSRAMInfo info;
    sm_malloc_stats_pool(&extmem_smalloc_pool, &info.m_total, &info.m_used, &info.m_free, &info.m_blockCount);
    return info;
  }

  size_t getDynamicPsramTotal()
  {
    size_t total = 0;
    size_t free = 0;
    sm_malloc_stats_pool(&extmem_smalloc_pool, &total, nullptr, &free, nullptr);
    return total;
  }

  size_t getDynamicUsedPsramInBytes()
  {
    size_t total = 0;
    size_t free = 0;
    size_t used = 0;
    sm_malloc_stats_pool(&extmem_smalloc_pool, &total, &used, &free, nullptr);
    return total;
  }

  size_t getDynamicAvailablePsramInBytes()
  {
    size_t total = 0;
    size_t free = 0;
    sm_malloc_stats_pool(&extmem_smalloc_pool, &total, nullptr, &free, nullptr);
    return free;
  }

  int getDynamicPsramBlockCount()
  {
    size_t total = 0;
    size_t free = 0;
    int blockCount = 0;
    sm_malloc_stats_pool(&extmem_smalloc_pool, &total, nullptr, &free, &blockCount);
    return blockCount;
  }
#endif
  
  // ---- memory infos about variables, arrays and functions ----
  // based on memoryTool from https://github.com/luni64/TeensyHelpers
  
  namespace implementation // do not use in user code
  {
    const ElementInfo getElememtInfo(const char* in_name, const void* in_begin, uint32_t in_elementSize, uint32_t in_elementCount)
    {
      ElementInfo info;
      info.m_name = in_name;
      info.m_size = in_elementSize * in_elementCount;
      info.m_startAddress = reinterpret_cast<uintptr_t>(in_begin);
      info.m_endAddress = info.m_startAddress + info.m_size - 1;
      
#if ARDUINO_TEENSY41
      if (info.m_startAddress >= getStaticPsramStart())
#else
      if (false)
#endif
      {
        info.m_location = "EXTMEM (PSRAM, not initialized)";
      }
      else if (info.m_startAddress >= getFlashStart())
      {
        info.m_location = "FLASH";
      }
      else if (info.m_startAddress >= getHeapStart())
      {
        info.m_location = "HEAP (RAM2)";
      }
      else if (info.m_startAddress >= getRamStart())
      {
        info.m_location = "DMAMEM (RAM2, not initialized)";
      }
      else if (info.m_startAddress >= getStackEnd())
      {
        info.m_location = "STACK (RAM1)";
      }
      else if (info.m_startAddress >= getUninitialisedDataStart())
      {
        info.m_location = "DTCM (RAM1, zeroed)";
      }
      else if (info.m_startAddress >= getInitialisedDataStart())
      {
        info.m_location = "DTCM (RAM1, initialized)";
      }
      else
      {
        info.m_location = "ITCM (RAM1, copied code from FLASH)";
      }
      
      return info;
    }
  }
}
