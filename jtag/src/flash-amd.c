/*
 * $Id$
 *
 * Flash driver for AMD Am29LV640D, Am29LV641D, Am29LV642D
 * Copyright (C) 2003 AH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by August Hörandl <august.hoerandl@gmx.at>
 *
 * Documentation:
 * [1] Advanced Micro Devices, "Am29LV640D/Am29LV641D",
 *     September 20, 2002     Rev B, 22366b8.pdf
 * [2] Advanced Micro Devices, "Am29LV642D",
 *     August 14, 2001    Rev A, 25022.pdf
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <flash/cfi.h>
#include <flash/intel.h>
#include <unistd.h>

#include "flash.h"

static int dbg = 0;

static int amd_flash_erase_block( parts *ps, uint32_t adr );
static int amd_flash_unlock_block( parts *ps, uint32_t adr );
static int amd_flash_program( parts *ps, uint32_t adr, uint32_t data );
static void amd_flash_read_array( parts *ps); 

/* autodetect, we can handle this chip */
static int 
amd_flash_autodetect( parts *ps, cfi_query_structure_t *cfi )
{
	return (cfi->identification_string.pri_id_code == CFI_VENDOR_AMD_SCS);
}

/*
 * check device status
 *   1/true   PASS
 *   0/false  FAIL
 */
/*
 * first implementation: see [1], page 29
 */
static int
amdstatus29( parts *ps, uint32_t adr, uint32_t data )
{
	int o = 2;
	int timeout;
	uint32_t dq7mask = ((1 << 7) << 16) + (1 << 7);
	uint32_t dq5mask = ((1 << 5) << 16) + (1 << 5);
	uint32_t bit7 = (data & (1 << 7)) != 0;
	uint32_t data1;

	for (timeout = 0; timeout < 100; timeout++) {
		data1 = bus_read( ps, adr << o );
		data1 = bus_read( ps, adr << o );
		if (dbg)
			printf( "amdstatus %d: %04X (%04X) = %04X\n", timeout, data1, (data1 & dq7mask), bit7 );
		if (((data1 & dq7mask) == dq7mask) == bit7)		/* FIXME: This looks non-portable */
			return 1;

		if ((data1 & dq5mask) == dq5mask)
			break;
		usleep( 100 );
	}

	data1 = bus_read( ps, adr << o );
	if (((data1 & dq7mask) == dq7mask) == bit7)			/* FIXME: This looks non-portable */
		return 1;

	return 0;
}

/*
 * second implementation: see [1], page 30
 */
static int
amdstatus( parts *ps, uint32_t adr, int data )
{
	int timeout;
	uint32_t togglemask = ((1 << 6) << 16) + (1 << 6); /* DQ 6 */
	/*  int dq5mask = ((1 << 5) << 16) + (1 << 5); DQ5 */

	for (timeout = 0; timeout < 100; timeout++) {
		uint32_t data1 = bus_read( ps, adr );
		uint32_t data2 = bus_read( ps, adr );

		/*printf("amdstatus %d: %04X/%04X   %04X/%04X \n", */
		/*	   timeout, data1, data2, (data1 & togglemask), (data2 & togglemask)); */
		if ( (data1 & togglemask) == (data2 & togglemask)) /* no toggle */
			return 1;

		/*    if ( (data1 & dq5mask) != 0 )   TODO */
		/*      return 0; */
		if (dbg) 
			printf( "amdstatus %d: %04X/%04X\n", timeout, data1, data2 );
		else
			printf( "." );
		usleep( 100 );
	}
	return 0;
}

static int
amdisprotected( parts *ps, uint32_t adr )
{
	uint32_t data;
	int o = 2;

	bus_write( ps, 0x0555 << o, 0x00aa00aa );	/* autoselect p29, sector erase */
	bus_write( ps, 0x02aa << o, 0x00550055 );
	bus_write( ps, 0x0555 << o, 0x00900090 );

	data = bus_read( ps, adr + (0x0002 << 2) );
	/* Read Array */
	amd_flash_read_array( ps ); /* AMD reset */

	return ((data & 0x00ff00ff) != 0);
}

static void
amd_flash_print_info( parts *ps )
{
	int o = 2;
	int mid, cid, prot;
	bus_write( ps, 0x0555 << o, 0x00aa00aa );	/* autoselect p29 */
	bus_write( ps, 0x02aa << o, 0x00550055 );
	bus_write( ps, 0x0555 << o, 0x00900090 );
	mid = bus_read( ps, 0x00 << o ) & 0xFFFF;
	cid = bus_read( ps, 0x01 << o ) & 0xFFFF;
	prot = bus_read( ps, 0x02 << o ) & 0xFF;
	amd_flash_read_array(ps); /* AMD reset */
	printf( "Chip: AMD Flash\n\tManufacturer: " );
	switch (mid) {
		case 0x0001:
			printf( "AMD" );
			break;
		default:
			printf( "Unknown manufacturer (ID 0x%04x)", mid );
			break;
	}
	printf( "\n\tChip: " );
	switch (cid) {
		case 0x22D7:
			printf( "Am29LV640D/Am29LV641D/Am29LV642D" );
			break;
		default:
			printf ( "Unknown (ID 0x%04x)", cid );
			break;
	}
	printf( "\n\tProtected: %04x\n", prot );
}

static int
amd_flash_erase_block( parts *ps, uint32_t adr )
{
	int o = 2;

	printf("flash_erase_block 0x%08X\n", adr);

	/*	printf("protected: %d\n", amdisprotected(ps, adr)); */

	bus_write( ps, 0x0555 << o, 0x00aa00aa ); /* autoselect p29, sector erase */
	bus_write( ps, 0x02aa << o, 0x00550055 );
	bus_write( ps, 0x0555 << o, 0x00800080 );
	bus_write( ps, 0x0555 << o, 0x00aa00aa );
	bus_write( ps, 0x02aa << o, 0x00550055 );
	bus_write( ps, adr, 0x00300030 );

	if (amdstatus(ps, adr, 0xffff)) {
		printf( "flash_erase_block 0x%08X DONE\n", adr );
		amd_flash_read_array( ps ); /* AMD reset */
		return 0;
	}
	printf( "flash_erase_block 0x%08X FAILED\n", adr );
	/* Read Array */
	amd_flash_read_array( ps ); /* AMD reset */

	return CFI_INTEL_ERROR_UNKNOWN;
}

static int
amd_flash_unlock_block( parts *ps, uint32_t adr )
{
	printf( "flash_unlock_block 0x%08X IGNORE\n", adr );
	return 0;
}

static int
amd_flash_program( parts *ps, uint32_t adr, uint32_t data )
{
	int o = 2;
	int status;

	if (dbg)
		printf("\nflash_program 0x%08X = 0x%08X\n", adr, data);

	bus_write( ps, 0x0555 << o, 0x00aa00aa ); /* autoselect p29, program */
	bus_write( ps, 0x02aa << o, 0x00550055 );
	bus_write( ps, 0x0555 << o, 0x00A000A0 );

	bus_write( ps, adr, data );
	status = amdstatus( ps, adr, data );
	/*	amd_flash_read_array(ps); */

	return !status;
}

static void
amd_flash_read_array( parts *ps)
{
	/* Read Array */
	bus_write( ps, 0x0, 0x00F000F0 ); /* AMD reset */
}

flash_driver_t amd_32_flash_driver = {
	4, /* buswidth */
	"AMD/Fujitsu Standard Command Set",
	"supported: AMD 29LV640D, 29LV641D, 29LV642D; 2x16 Bit",
	amd_flash_autodetect,
	amd_flash_print_info,
	amd_flash_erase_block,
	amd_flash_unlock_block,
	amd_flash_program,
	amd_flash_read_array,
};