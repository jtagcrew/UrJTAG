/*
 * $Id$
 *
 * Copyright (C) 2002 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2002.
 *
 * Documentation:
 * [1] Advanced Micro Devices, "Common Flash Memory Interface Specification Release 2.0",
 *     December 1, 2001
 * [2] Intel Corporation, "Intel PXA250 and PXA210 Application Processors
 *     Developer's Manual", February 2002, Order Number: 278522-001
 * [3] Intel Corporation, "Common Flash Interface (CFI) and Command Sets
 *     Application Note 646", April 2000, Order Number: 292204-004
 * [4] Advanced Micro Devices, "Common Flash Memory Interface Publication 100 Vendor & Device
 *     ID Code Assignments", December 1, 2001, Volume Number: 96.1
 *
 */

#include <stdint.h>
#include <string.h>
#include <flash/cfi.h>
#include <flash/intel.h>
#include <std/mic.h>

#include "part.h"
#include "bus.h"

cfi_query_structure_t *detect_cfi( parts * );

void
detectflash( parts *ps )
{
	part *p = ps->parts[0];
	int o = 0;
	cfi_query_structure_t *cfi;

	if (!bus_driver) {
		printf( "Error: Missing bus_driver!\n" );
		return;
	}

	printf( "Note: Supported configuration is 2 x 16 bit only\n" );

	switch (bus_width( ps )) {
		case 16:
			o = 1;
			break;
		case 32:
			o = 2;
			break;
		default:
			printf( "Error: Unknown bus width!\n" );
			return;
	}

	/* EXTEST */
	part_set_instruction( p, "EXTEST" );
	parts_shift_instructions( ps );

	cfi = detect_cfi( ps );
	if (!cfi) {
		printf( "Flash not found!\n" );
		return;
	}

	/* detect CFI capable devices */
	/* TODO: Low chip only */
	/* see 3.3.2 in [1] */
	printf( "CFI Query Identification String:\n" );
	printf( "\tPrimary Vendor Command Set and Control Interface ID Code: 0x%04X ", cfi->identification_string.pri_id_code );
	/* see Section 1. in [4] */
	switch (cfi->identification_string.pri_id_code) {
		case CFI_VENDOR_NULL:
			printf( "(null)\n" );
			break;
		case CFI_VENDOR_INTEL_ECS:
			printf( "(Intel/Sharp Extended Command Set)\n" );
			break;
		case CFI_VENDOR_AMD_SCS:
			printf( "(AMD/Fujitsu Standard Commanf Set)\n" );
			break;
		case CFI_VENDOR_INTEL_SCS:
			printf( "(Intel Standard Command Set)\n" );
			break;
		case CFI_VENDOR_AMD_ECS:
			printf( "(AMD/Fujitsu Extended Command Set)\n" );
			break;
		case CFI_VENDOR_MITSUBISHI_SCS:
			printf( "(Mitsubishi Standard Command Set)\n" );
			break;
		case CFI_VENDOR_MITSUBISHI_ECS:
			printf( "(Mitsubishi Extended Command Set)\n" );
			break;
		case CFI_VENDOR_SST_PWCS:
			printf( "(Page Write Command Set)\n" );
			break;
		default:
			printf( "(unknown!!!)\n" );
			break;
	}
	printf( "\tAddress of Primary Algorithm extended Query table: P = 0x????\n" );
	printf( "\tAlternate Vendor Command Set and Control Interface ID Code: 0x%04X ", cfi->identification_string.alt_id_code );
	switch (cfi->identification_string.alt_id_code) {
		case CFI_VENDOR_NULL:
			printf( "(null)\n" );
			break;
		case CFI_VENDOR_INTEL_ECS:
			printf( "(Intel/Sharp Extended Command Set)\n" );
			break;
		case CFI_VENDOR_AMD_SCS:
			printf( "(AMD/Fujitsu Standard Commanf Set)\n" );
			break;
		case CFI_VENDOR_INTEL_SCS:
			printf( "(Intel Standard Command Set)\n" );
			break;
		case CFI_VENDOR_AMD_ECS:
			printf( "(AMD/Fujitsu Extended Command Set)\n" );
			break;
		case CFI_VENDOR_MITSUBISHI_SCS:
			printf( "(Mitsubishi Standard Command Set)\n" );
			break;
		case CFI_VENDOR_MITSUBISHI_ECS:
			printf( "(Mitsubishi Extended Command Set)\n" );
			break;
		case CFI_VENDOR_SST_PWCS:
			printf( "(Page Write Command Set)\n" );
			break;
		default:
			printf( "(unknown!!!)\n" );
			break;
	}
	printf( "\tAddress of Alternate Algorithm extended Query table: A = 0x????\n" );

	/* see 3.3.3 in [1] */
	printf( "CFI Query System Interface Information:\n" );
	printf( "\tVcc Logic Supply Minimum Write/Erase voltage: %d mV\n", cfi->system_interface_info.vcc_min_wev );
	printf( "\tVcc Logic Supply Maximum Write/Erase voltage: %d mV\n", cfi->system_interface_info.vcc_max_wev );
	printf( "\tVpp [Programming] Logic Supply Minimum Write/Erase voltage: %d mV\n", cfi->system_interface_info.vpp_min_wev );
	printf( "\tVpp [Programming] Logic Supply Maximum Write/Erase voltage: %d mV\n", cfi->system_interface_info.vpp_max_wev );
	printf( "\tTypical timeout per single byte/word write: %d us\n", cfi->system_interface_info.typ_single_write_timeout );
	printf( "\tTypical timeout for minimum-size buffer write: %d us\n", cfi->system_interface_info.typ_buffer_write_timeout );
	printf( "\tTypical timeout per individual block erase: %d ms\n", cfi->system_interface_info.typ_block_erase_timeout );
	printf( "\tTypical timeout for full chip erase: %d ms\n", cfi->system_interface_info.typ_chip_erase_timeout );
	printf( "\tMaximum timeout for byte/word write: %d us\n", cfi->system_interface_info.max_single_write_timeout );
	printf( "\tMaximum timeout for buffer write: %d us\n", cfi->system_interface_info.max_buffer_write_timeout );
	printf( "\tMaximum timeout per individual block erase: %d ms\n", cfi->system_interface_info.max_block_erase_timeout );
	printf( "\tMaximum timeout for chip erase: %d ms\n", cfi->system_interface_info.max_chip_erase_timeout );

	/* see 3.3.4 in [1] */
	printf( "Device Geometry Definition:\n" );
	printf( "\tDevice Size: %d B\n", cfi->device_geometry.device_size );
	printf( "\tFlash Device Interface description: 0x%04X ", cfi->device_geometry.device_interface );
	/* see Section 2. in [4] */
	switch (cfi->device_geometry.device_interface) {
		case CFI_INTERFACE_X8:
			printf( "(x8)\n" );
			break;
		case CFI_INTERFACE_X16:
			printf( "(x16)\n" );
			break;
		case CFI_INTERFACE_X8_X16:
			printf( "(x8/x16)\n" );
			break;
		case CFI_INTERFACE_X32:
			printf( "(x32)\n" );
			break;
		case CFI_INTERFACE_X16_X32:
			printf( "(x16/x32)\n" );
			break;
		default:
			printf( "(unknown!!!)\n" );
			break;
	}
	printf( "\tMaximum number of bytes in multi-byte write: %d\n", cfi->device_geometry.max_bytes_write );
	printf( "\tNumber of Erase Block Regions within device: %d\n", cfi->device_geometry.number_of_erase_regions );
	printf( "\tErase Block Region Information:\n" );
	{
		int i;

		for (i = 0; i < cfi->device_geometry.number_of_erase_regions; i++) {
			printf( "\t\tRegion %d:\n", i );
			printf( "\t\t\tErase Block Size: %d\n", cfi->device_geometry.erase_block_regions[i].erase_block_size );
			printf( "\t\t\tNumber of Erase Blocks: %d\n", cfi->device_geometry.erase_block_regions[i].number_of_erase_blocks );
		}
	}

	/* Intel Primary Algorithm Extended Query Table - see Table 5. in [3] */
	/* TODO */

	/* Clear Status Register */
	bus_write( ps, 0 << o, 0x00500050 );

	/* Read Identifier Command */
	bus_write( ps, 0 << 0, 0x00900090 );

	switch (bus_read( ps, 0x00 << o ) & 0xFF) {
		case STD_MIC_INTEL:
			printf( "Manufacturer: %s\n", STD_MICN_INTEL );
			break;
		default:
			printf( "Unknown manufacturer!\n" );
			break;
	}

	printf( "Chip: " );
	switch (bus_read( ps, 0x01 << o ) & 0xFFFF) {
		case 0x0016:
			printf( "28F320J3A\n" );
			break;
		case 0x0017:
			printf( "28F640J3A\n" );
			break;
		case 0x0018:
			printf( "28F128J3A\n" );
			break;
		case 0x8801:
			printf( "28F640K3\n" );
			break;
		case 0x8802:
			printf( "28F128K3\n" );
			break;
		case 0x8803:
			printf( "28F256K3\n" );
			break;
		case 0x8805:
			printf( "28F640K18\n" );
			break;
		case 0x8806:
			printf( "28F128K18\n" );
			break;
		case 0x8807:
			printf( "28F256K18\n" );
			break;
		default:
			printf( "Unknown!\n" );
			break;
	}

	/* Read Array */
	bus_write( ps, 0 << o, 0x00FF00FF );
}

void
readmem( parts *ps, FILE *f, uint32_t addr, uint32_t len )
{
	part *p = ps->parts[0];
	int step = 0;
	uint32_t a;

	if (!bus_driver) {
		printf( "Error: Missing bus_driver!\n" );
		return;
	}

	step = bus_width( ps ) / 8;

	if (step == 0) {
		printf( "Unknown bus width!\n" );
		return;
	}

	/* EXTEST */
	part_set_instruction( p, "EXTEST" );
	parts_shift_instructions( ps );

	addr = addr & (~(step - 1));
	len = (len + step - 1) & (~(step - 1));

	printf( "address: 0x%08X\n", addr );
	printf( "length:  0x%08X\n", len );

	if (len == 0) {
		printf( "length is 0.\n" );
		return;
	}

	printf( "reading:\n" );
	bus_read_start( ps, addr );
	for (a = addr + step; a <= addr + len; a += step) {
		uint32_t d;

		printf( "addr: 0x%08X\r", a );

		if (a < addr + len)
			d = bus_read_next( ps, a );
		else
			d = bus_read_end( ps );
		fwrite( &d, step, 1, f );
	}

	printf( "\nDone.\n" );
}
