#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <new>
#include <sstream>

#include "Atom/Debug.h"

#ifdef WIN32
    #pragma warning ( disable : 4786 )
    #pragma warning ( disable : 4290 )
    #pragma warning ( disable : 4503 )
    #pragma warning ( disable : 4267 )  // conversion from * to *, possible loss of data

    #define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
    #define mmin std::min
    #define mmax std::max

    #ifdef _DEBUG
        #define PAUSE getchar();
    #endif

#else
    #ifndef UNIX
        #define UNIX
    #endif

    #ifdef GCC
        #define mmin(i, j) (i <? j)
        #define mmax(i, j) (i >? j)
    #else
        #define mmin(i, j) (((i) < (j))?(i):(j))
        #define mmax(i, j) (((i) > (j))?(i):(j))
    #endif
#endif

#ifndef PAUSE
    #define PAUSE
#endif

/** Default for appending underscore to fortran subroutines. */
#ifndef _FUS
    #define _FUS 1
#endif

/** I/O streams */
extern std::ostream* outstream;
extern std::ostream* logstream;
extern std::ostream* errstream;

extern Debug DebugOptions;

/** MPI */
extern unsigned int NumProcessors;
extern unsigned int ProcessorRank;

extern std::string itoa(int value, unsigned int base = 10);

/** Change endianness when reading. */
//#define fread fread_other_endian
extern size_t fread_other_endian(void* ptr, size_t size, size_t count, FILE * fp);

#endif
