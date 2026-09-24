#ifndef CONTEST_FREEMODBUS_PORT_H
#define CONTEST_FREEMODBUS_PORT_H

#include <assert.h>
#include <stdint.h>

#define INLINE inline
#define PR_BEGIN_EXTERN_C extern "C" {
#define PR_END_EXTERN_C }
#define ENTER_CRITICAL_SECTION()
#define EXIT_CRITICAL_SECTION()

typedef uint8_t BOOL;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef uint16_t USHORT;
typedef int16_t SHORT;

/* Match FatFs integer.h exactly; both types are 32-bit on ARM Compiler 6. */
typedef unsigned long ULONG;
typedef long LONG;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* CONTEST_FREEMODBUS_PORT_H */
