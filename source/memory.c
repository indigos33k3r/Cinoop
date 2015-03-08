#include <stdio.h>

#include "registers.h"
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "debug.h"

#include "memory.h"

const unsigned char ioReset[0x100] = {
	0x0F, 0x00, 0x7C, 0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
	0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
	0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
	0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xC1, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,
	0xF8, 0xFF, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	0x45, 0xEC, 0x52, 0xFA, 0x08, 0xB7, 0x07, 0x5D, 0x01, 0xFD, 0xC0, 0xFF, 0x08, 0xFC, 0x00, 0xE5,
	0x0B, 0xF8, 0xC2, 0xCE, 0xF4, 0xF9, 0x0F, 0x7F, 0x45, 0x6D, 0x3D, 0xFE, 0x46, 0x97, 0x33, 0x5E,
	0x08, 0xEF, 0xF1, 0xFF, 0x86, 0x83, 0x24, 0x74, 0x12, 0xFC, 0x00, 0x9F, 0xB4, 0xB7, 0x06, 0xD5,
	0xD0, 0x7A, 0x00, 0x9E, 0x04, 0x5F, 0x41, 0x2F, 0x1D, 0x77, 0x36, 0x75, 0x81, 0xAA, 0x70, 0x3A,
	0x98, 0xD1, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0x0B, 0x00
};

unsigned char cart[0x8000];
unsigned char sram[0x2000];
unsigned char io[0x100];
unsigned char vram[0x2000];
unsigned char oam[0x100];
unsigned char wram[0x2000];
unsigned char hram[0x80];

void copy(unsigned short destination, unsigned short source, size_t length) {
	unsigned int i;
	for(i = length; i > 0; i--) writeByte(destination + i, readByte(source + i));
}

unsigned char readByte(unsigned short address) {
	// Set read breakpoints here
	//if(address == 0x0300) {
	//	realtimeDebugEnable = 1;
	//}
	
	if(address <= 0x7fff)
		return cart[address];
	
	else if(address >= 0xa000 && address <= 0xbfff)
		return sram[address - 0xa000];
	
	else if(address >= 0x8000 && address <= 0x9fff)
		return vram[address - 0x8000];
	
	else if(address >= 0xc000 && address <= 0xdfff)
		return wram[address - 0xc000];
	
	else if(address >= 0xe000 && address <= 0xfdff)
		return wram[address - 0xe000];
	
	else if(address >= 0xfe00 && address <= 0xfeff)
		return oam[address - 0xfe00];
	
	else if(address >= 0xff80 && address <= 0xfffe)
		return hram[address - 0xff80];
	
	else if(address == 0xff40) return gpu.control;
	else if(address == 0xff42) return gpu.scrollY;
	else if(address == 0xff43) return gpu.scrollX;
	else if(address == 0xff44) return gpu.scanline; // read only
	
	//else if(address >= 0xff00 && address <= 0xff7f)
	//	return io[address - 0xff00];
	
	else if(address == 0xff0f) return interrupt.flags;
	else if(address == 0xffff) return interrupt.enable;
	
	return 0;
}

unsigned short readShort(unsigned short address) {
	return readByte(address) | (readByte(address + 1) << 8);
}

unsigned short readShortFromStack(void) {
	unsigned short value = readShort(registers.sp);
	registers.sp += 2;
	
	#ifdef DEBUG_STACK
		printf("Stack read 0x%04x\n", value);
	#endif
	
	return value;
}

void writeByte(unsigned short address, unsigned char value) {
	// Set write breakpoints here
	//if(address == 0xffa6) {
		//realtimeDebugEnable = 1;
	//}
	
	// Block writes to ff80
	if(tetrisPatch && address == 0xff80) return;
	
	if(address >= 0xa000 && address <= 0xbfff)
		sram[address - 0xa000] = value;
	
	else if(address >= 0x8000 && address <= 0x9fff) {
		vram[address - 0x8000] = value;
		updateTile(address, value);
	}
	
	if(address >= 0xc000 && address <= 0xdfff)
		wram[address - 0xc000] = value;
	
	else if(address >= 0xe000 && address <= 0xfdff)
		wram[address - 0xe000] = value;
	
	else if(address >= 0xfe00 && address <= 0xfeff) {
		oam[address - 0xfe00] = value;
		//printf("wrote 0x%04x\n", address);
		//updateOAM(address, value);
	}
	
	else if(address >= 0xff80 && address <= 0xfffe)
		hram[address - 0xff80] = value;
	
	else if(address == 0xff40) gpu.control = value;
	else if(address == 0xff42) gpu.scrollY = value;
	else if(address == 0xff43) gpu.scrollX = value;
	
	else if(address == 0xff46) {
		copy(0xfe00, value << 8, 160);
		ticks += 671;
	}
	
	else if(address == 0xff47) gpu.bgPalette = value; // write only
	else if(address == 0xff48) gpu.spritePalette[0] = value; // write only
	else if(address == 0xff49) gpu.spritePalette[1] = value; // write only
	
	else if(address >= 0xff00 && address <= 0xff7f)
		io[address - 0xff00] = value;
	
	else if(address == 0xff0f) interrupt.flags = value;
	else if(address == 0xffff) interrupt.enable = value;
}

void writeShort(unsigned short address, unsigned short value) {
	writeByte(address, (unsigned char)(value & 0x00ff));
	writeByte(address + 1, (unsigned char)((value & 0xff00) >> 8));
}

void writeShortToStack(unsigned short value) {
	registers.sp -= 2;
	writeShort(registers.sp, value);
	
	#ifdef DEBUG_STACK
		printf("Stack write 0x%04x\n", value);
	#endif
}
