/*
 * ex_memtest.c
   Copyright (C) 2020       Thomas Sowell
   Copyright (C) 2022, 2023 Eric Fradella

   This application illustrates the use of functions related to detecting the
   size of system memory and altering a program's behavior to suit the running
   system's configuration.

   Implemented is a memory test utility for Dreamcast consoles (both stock
   16MB systems and modified 32MB systems), based on public domain code by
   Michael Barr in memtest.c found here:
   https://barrgroup.com/embedded-systems/how-to/memory-test-suite-c

   Example output on a functional 32MB-modified Dreamcast:

   Beginning memtest routine...
    Base address: 0x8c100000
    Number of bytes to test: 32440320
     memTestDataBus: PASS
     memTestAddressBus: PASS
     memTestDevice: PASS
   Test passed!
*/

/**********************************************************************
 *
 * Filename:    memtest.h
 *
 * Description: Memory-testing module API.
 *
 * Notes:       The memory tests can be easily ported to systems with
 *              different data bus widths by redefining 'datum' type.
 *
 *
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include <stdint.h>

#ifndef NULL
#define NULL  (void *) 0
#endif

typedef uint32_t datum;

/*
 * Function prototypes.
 */
datum   memTestDataBus(volatile datum * address);
datum * memTestAddressBus(volatile datum * baseAddress, unsigned long nBytes);
datum * memTestDevice(volatile datum * baseAddress, unsigned long nBytes);

/**********************************************************************
 *
 * Filename:    memtest.c
 *
 * Description: General-purpose memory testing functions.
 *
 * Notes:       This software can be easily ported to systems with
 *              different data bus widths by redefining 'datum'.
 *
 *
 * Copyright (c) 1998 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

/**********************************************************************
 *
 * Function:    memTestDataBus()
 *
 * Description: Test the data bus wiring in a memory region by
 *              performing a walking 1's test at a fixed address
 *              within that region.  The address (and hence the
 *              memory region) is selected by the caller.
 *
 * Notes:
 *
 * Returns:     0 if the test succeeds.
 *              A non-zero result is the first pattern that failed.
 *
 **********************************************************************/
datum
memTestDataBus(volatile datum * address)
{
    datum pattern;


    /*
     * Perform a walking 1's test at the given address.
     */
    for (pattern = 1; pattern != 0; pattern <<= 1)
    {
        /*
         * Write the test pattern.
         */
        *address = pattern;

        /*
         * Read it back (immediately is okay for this test).
         */
        if (*address != pattern)
        {
            return (pattern);
        }
    }

    return (0);

}   /* memTestDataBus() */


/**********************************************************************
 *
 * Function:    memTestAddressBus()
 *
 * Description: Test the address bus wiring in a memory region by
 *              performing a walking 1's test on the relevant bits
 *              of the address and checking for aliasing. This test
 *              will find single-bit address failures such as stuck
 *              -high, stuck-low, and shorted pins.  The base address
 *              and size of the region are selected by the caller.
 *
 * Notes:       For best results, the selected base address should
 *              have enough LSB 0's to guarantee single address bit
 *              changes.  For example, to test a 64-Kbyte region,
 *              select a base address on a 64-Kbyte boundary.  Also,
 *              select the region size as a power-of-two--if at all
 *              possible.
 *
 * Returns:     NULL if the test succeeds.
 *              A non-zero result is the first address at which an
 *              aliasing problem was uncovered.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/
datum *
memTestAddressBus(volatile datum * baseAddress, unsigned long nBytes)
{
    unsigned long addressMask = (nBytes/sizeof(datum) - 1);
    unsigned long offset;
    unsigned long testOffset;

    datum pattern     = (datum) 0xAAAAAAAA;
    datum antipattern = (datum) 0x55555555;


    /*
     * Write the default pattern at each of the power-of-two offsets.
     */
    for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
    {
        baseAddress[offset] = pattern;
    }

    /*
     * Check for address bits stuck high.
     */
    testOffset = 0;
    baseAddress[testOffset] = antipattern;

    for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
    {
        if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
    }

    baseAddress[testOffset] = pattern;

    /*
     * Check for address bits stuck low or shorted.
     */
    for (testOffset = 1; (testOffset & addressMask) != 0; testOffset <<= 1)
    {
        baseAddress[testOffset] = antipattern;

		if (baseAddress[0] != pattern)
		{
			return ((datum *) &baseAddress[testOffset]);
		}

        for (offset = 1; (offset & addressMask) != 0; offset <<= 1)
        {
            if ((baseAddress[offset] != pattern) && (offset != testOffset))
            {
                return ((datum *) &baseAddress[testOffset]);
            }
        }

        baseAddress[testOffset] = pattern;
    }

    return (NULL);

}   /* memTestAddressBus() */


/**********************************************************************
 *
 * Function:    memTestDevice()
 *
 * Description: Test the integrity of a physical memory device by
 *              performing an increment/decrement test over the
 *              entire region.  In the process every storage bit
 *              in the device is tested as a zero and a one.  The
 *              base address and the size of the region are
 *              selected by the caller.
 *
 * Notes:
 *
 * Returns:     NULL if the test succeeds.
 *
 *              A non-zero result is the first address at which an
 *              incorrect value was read back.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/
datum *
memTestDevice(volatile datum * baseAddress, unsigned long nBytes)
{
    unsigned long offset;
    unsigned long nWords = nBytes / sizeof(datum);

    datum pattern;
    datum antipattern;


    /*
     * Fill memory with a known pattern.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        baseAddress[offset] = pattern;
    }

    /*
     * Check each location and invert it for the second pass.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }

        antipattern = ~pattern;
        baseAddress[offset] = antipattern;
    }

    /*
     * Check each location for the inverted pattern and zero it.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        antipattern = ~pattern;
        if (baseAddress[offset] != antipattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
    }

    return (NULL);

}   /* memTestDevice() */

/* End memtest.h/.c, begin libronin memtest example */

#include <string.h>
#include "notlibc.h"

/* Leave space at the beginning and end of memory for this program and for the
 * stack. Dreamcast applications are loaded at 0x8c000000 so we leave 0x100000
 * bytes past that for the program itself and leave 65536 bytes at the top of
 * memory for the stack. */
#define SAFE_AREA     0x100000
#define STACK_SIZE    65536
#define BASE_ADDRESS  (volatile datum *) (0x8c000000 + SAFE_AREA)

/* Define the number of bytes to be tested. libronin provides the
 * macros HW_MEM_16 and HW_MEM_32 which describe the number of bytes
 * available in standard supported console configurations (16777216
 * and 33554432, respectively). */
#define NUM_BYTES_32  (HW_MEM_32 - SAFE_AREA - STACK_SIZE)
#define NUM_BYTES_16  (HW_MEM_16 - SAFE_AREA - STACK_SIZE)

int main(int argc, char **argv) {
    uint32_t error, data, *address;
    unsigned long num_bytes;
    error = 0;
    char string[20] = {0};

    serial_init(57600);
    usleep(1500000);

    /* The HW_MEMSIZE can be called to retrieve the system's memory size.
     * _mem_top defines the top address of memory.
     * 0x8d000000 if 16MB console, 0x8e000000 if 32MB */
    serial_puts("\nThis console has ");
    itoa(HW_MEMSIZE, string, 10);
    serial_puts(string);
    serial_puts(" bytes of system memory,\n with top of memory located at ");
    itoa(_mem_top - 1, string, 16);
    serial_puts(string);
    serial_puts(".\n\n");

    /* The DBL_MEM boolean macro is provided as an easy, concise
     * way to determine if extra system RAM is available */
    num_bytes = DBL_MEM ? NUM_BYTES_32 : NUM_BYTES_16;

    serial_puts("Beginning memtest routine...\n");
    serial_puts(" Base address: ");
    itoa(BASE_ADDRESS, string, 16);
    serial_puts(string);
    serial_puts("\n Number of bytes to test: ");
    itoa(num_bytes, string, 10);
    serial_puts(string);
    serial_puts("\n");

    /* Now we run the test routines provided in memtest.c
     * Each routine returns zero if the routine passes,
     * else it returns the address of failure.
     * First, let's test the data bus. */
    serial_puts("  memTestDataBus: ");
    serial_flush();
    data = memTestDataBus(BASE_ADDRESS);

    if(data != 0) {
        serial_puts("FAIL: ");
        itoa(data, string, 16);
        serial_puts(string);
        serial_puts("\n");
        error = 1;
    }
    else {
        serial_puts("PASS\n");
    }

    serial_flush();

    /* Now we test the address bus. */
    serial_puts("  memTestAddressBus: ");
    serial_flush();
    address = memTestAddressBus(BASE_ADDRESS, num_bytes);

    if(address != NULL) {
        serial_puts("FAIL: ");
        itoa(address, string, 16);
        serial_puts(string);
        serial_puts("\n");
        error = 1;
    }
    else {
        serial_puts("PASS\n");
    }

    serial_flush();

    /* And now, we test the memory itself. */
    serial_puts("  memTestDevice: ");
    serial_flush();
    address = memTestDevice(BASE_ADDRESS, num_bytes);

    if(address != NULL) {
        serial_puts("FAIL: ");
        itoa(address, string, 16);
        serial_puts(string);
        serial_puts("\n");
        error = 1;
    }
    else {
        serial_puts("PASS\n");
    }

    serial_flush();

    /* Test completed, return final result */
    serial_puts("Test ");
    serial_puts(error ? "failed.\n" : "passed!\n");
    serial_flush();
    usleep(2000);
    return error;
}
