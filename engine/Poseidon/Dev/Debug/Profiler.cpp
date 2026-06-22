#include <Poseidon/Core/Application.hpp>

namespace Poseidon::Dev
{

#if PROFILE

#include <class/debugLog.hpp>
#include <class/array.hpp>
#include <fstream.h>
#include <stdlib.h>
#include <Poseidon/IO/MapFile.hpp>

extern "C" void __cdecl _penter();
extern "C" void __cdecl _pexit();

static void AddStatistics(int timeLo, int timeHi, int function);

// note: due to static storage
// our _pentry/_pexit profiler is not multithreading safe
// take care not to use multithreading when profiling

#define LIMIT_LEVEL 0

int profStack[64 * 1024];
int* profEsp = profStack;

void* retAddr;

int PEntryDisable = 1;
#if LIMIT_LEVEL
int PEntryLevel = LIMIT_LEVEL;
#endif

LONGLONG Slack = 0;

// there is some slack neccessary for an average _pentry/_pexit pair
#define SLACK_ESTIM (120)
// most of the slack is counted for a caller of an analysed function
#define SLACK_CALLER (100)

#define SYNC 0
#define CPUID __asm __emit 0fh __asm __emit 0a2h
#define RDTSC __asm __emit 0fh __asm __emit 031h

__declspec(naked) void __cdecl _penter()
{
    __asm
    {
// caution: this is not multithreading safe!!!
#if LIMIT_LEVEL
		test PEntryLevel,~0
		jl End
#endif

		test PEntryDisable,~0
		jne End

			push eax
			push ebx
			push ecx
			push edx

#define FrameLen 16
			mov ebx,profEsp
			mov edx,FrameLen+4[esp] // return address of analysed function
			mov ecx,FrameLen[esp] // start of analysed function +5B

        // push return address (function address+??) to our data stack		
			mov [ebx],edx
			mov [ebx]+12,ecx

// use RDTSC to get start time
#if SYNC
				xor eax,eax
				CPUID
#endif
			RDTSC

			mov ebx,profEsp
			mov [ebx+4],eax

			mov [ebx+8],edx
#if SLACK_ESTIM
				mov eax,dword ptr Slack
				mov edx,dword ptr Slack+4
				mov [ebx+16],eax
				mov [ebx+20],edx
#endif

			add ebx,24

			mov edx,offset _pexit

                // we have to modify return address to provide our own _pexit function
			mov FrameLen+4[esp],edx

			mov profEsp,ebx // save stack pointer

#if LIMIT_LEVEL
				dec PEntryLevel
#endif

			pop edx
			pop ecx
			pop ebx
			pop eax
		End:
		ret
    }
}

__declspec(naked) void __cdecl _pexit()
{
    __asm
    {
		push eax
		push ebx

		push ecx
		push edx

		push ebp

#if SYNC
			xor eax,eax
			CPUID
#endif
		RDTSC

		mov ebp,profEsp
		sub ebp,24
		mov ecx,[ebp]
		mov retAddr,ecx

		sub eax,[ebp+4] // get _pentry RDTSC data
		sbb edx,[ebp+8] // this should be 0 (if not overflowed)

		mov ecx,[ebp+12]

		mov profEsp,ebp

		sub ecx,5

		push ecx // push function address (argument 3)
		mov ecx,PEntryDisable
		cmp ecx,0
		jne NoStat 
			inc ecx
			mov PEntryDisable,ecx

// each _pentry/_pexit pair means some slack
#if SLACK_ESTIM
				mov ecx,dword ptr Slack
				mov ebx,dword ptr Slack+4
				add ecx,SLACK_ESTIM
				adc ebx,0
				mov dword ptr Slack,ecx
				mov dword ptr Slack+4,ebx

				sub ecx,[ebp+16] // slack from _pentry
				sbb ebx,[ebp+20] // slack from _pentry

				sub ecx,SLACK_CALLER // some of the slack is for the caller only
				sbb ebx,0

				sub eax,ecx // subtract slack from time
				sbb edx,ebx
#endif
			
			push edx
			push eax
             // save eax time to function ecx statistics
			call AddStatistics
			pop eax
			pop edx

			dec PEntryDisable
#if LIMIT_LEVEL
			inc PEntryLevel
#endif
		NoStat:
		pop ecx

		pop ebp
				
		pop edx
		pop ecx
		pop ebx
		pop eax
		jmp [retAddr]
    }
}

struct FuncStat
{
    int address; // reference only - not used during profiling
    int count;
    LARGE_INTEGER time;

    FuncStat(int at = 0)
    {
        address = at;
        count = 0;
        time.LowPart = time.HighPart = 0;
    }
};

#define F_ALIGN_LOG (4) // MS VC++ aligns all functions at 16B boundary

#define CODE_SIZE (1024 * 1024)
#define STAT_SIZE (CODE_SIZE >> F_ALIGN_LOG)

class Profiler : protected MapFile
{
  private:
    int _start;
    FuncStat _stats[STAT_SIZE];
    bool _reportDone;

  public:
    Profiler();
    ~Profiler();
    void AddStatistics(int timeLo, int timeHi, int function);
    void Enable();
    void Disable();

    void Start(); // prepare
    void End();   // show statistics
};

static Profiler ProfInstance; // static init/uninit

inline void Profiler::AddStatistics(int timeLo, int timeHi, int function)
{
    function >>= F_ALIGN_LOG;
    if (_start == 0)
        _start = function;
    int offset = function - _start;
    if (offset < 0)
    {
        if (offset > -STAT_SIZE)
        {
            memmove(_stats - offset, _stats, (STAT_SIZE + offset) * sizeof(*_stats));
            for (int i = 0; i < -offset; i++)
            {
                _stats[i] = FuncStat();
            }
        }
        _start += offset;
        offset = 0;
    }
    if (offset < STAT_SIZE)
    {
        FuncStat& stat = _stats[offset];
        stat.count++;
        LARGE_INTEGER time;
        time.LowPart = timeLo;
        time.HighPart = timeHi;
        stat.time.QuadPart += time.QuadPart;
    }
}

Profiler::Profiler()
{
    PEntryDisable = 1; // start disabled
    _start = 0;
    Start();
}

static int CmpStats(const void* s0, const void* s1)
{
    const FuncStat* f0 = static_cast<const FuncStat*>(s0);
    const FuncStat* f1 = static_cast<const FuncStat*>(s1);
    LONGLONG diff = f0->time.QuadPart - f1->time.QuadPart;
    if (diff < 0)
        return +1;
    if (diff > 0)
        return -1;
    return 0;
}

Profiler::~Profiler()
{
    Disable();
    End();
}

void Profiler::Enable()
{
    PEntryDisable--;
}
void Profiler::Disable()
{
    PEntryDisable++;
}

void Profiler::Start()
{
    _start = 0;
    memset(_stats, 0, sizeof(_stats));
    _reportDone = false;
}

void Profiler::End()
{
    if (_reportDone)
        return;
    ParseMapFile();
    _reportDone = true;
    int i;
    int nStats;
    LONGLONG maxTime = 0;
    for (i = 0; i < STAT_SIZE; i++)
    {
        FuncStat& stat = _stats[i];
        stat.address = (_start + i) << F_ALIGN_LOG;
        if (stat.time.QuadPart != 0)
            nStats = i + 1;
        if (stat.time.QuadPart > maxTime)
            maxTime = stat.time.QuadPart;
    }
    qsort(_stats, nStats, sizeof(*_stats), CmpStats);
    double invMax = 100.0 / maxTime;

    LOG_DEBUG(Core, "----------------------------------------------------------");
    LOG_DEBUG(Core, "{:>40} {:>10} {:>5}", "Name", "Count", "Time");
    LOG_DEBUG(Core, "----------------------------------------------------------");

    int penterAddr = (int)_penter;
    int penterMap = PhysicalAddress("__penter");
    int offset = penterMap - penterAddr;
    for (i = 0; i < nStats; i++)
    {
        FuncStat& stat = _stats[i];
        if (stat.count <= 0)
            continue;
        double relTime = (double)stat.time.QuadPart * invMax;
        if (relTime < 0.5)
            continue;
        const char* name = MapNameFromPhysical(stat.address + offset);
        LOG_DEBUG(Core, "{:>40} {:10} {:.1f}%", name, stat.count, relTime);
    }
}

static void AddStatistics(int timeLo, int timeHi, int function)
{
    ProfInstance.AddStatistics(timeLo, timeHi, function);
}

void EnableProfiler()
{
    ProfInstance.Start();
    ProfInstance.Enable();
}

void DisableProfiler()
{
    ProfInstance.Disable();
    ProfInstance.End();
}

#else

void EnableProfiler() {}
void DisableProfiler() {}

#endif

} // namespace Poseidon::Dev
