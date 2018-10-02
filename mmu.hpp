#pragma once

namespace MMU
{
extern u32 ignoreBadWrites;

extern u8 flash[0x40000];
extern u8 sram[0x8000];
extern u8 sram1[0x800];
extern u8 usbsram[0x800];

extern u8 eeprom[0x1000];

struct Register {
    u32 *value;
    const char *name;
    u32 (*read)( u32, u32 );
    u32 (*write)( u32, u32, u32 );
    u32 mute;
};

    u32 defaultRead( u32 v, u32 addr );
    u32 defaultWrite( u32 v, u32 ov, u32 addr );
    u32 nopWrite( u32 v, u32 ov, u32 addr );
    u32 readHole( u32 v, u32 addr );
    u32 writeHole( u32 v, u32 ov, u32 addr );
    u32 dbgRead( u32 v, u32 addr );
    u32 dbgWrite( u32 v, u32 ov, u32 addr );

#define MMUREGVRW( name, read, write ) { &MMU::_reserved, #name, &read, &write }
    
#define MMUREGRW( name, read, write ) { &name, #name, &read, &write }

#define MMUREGRO( name, read ) { &name, #name, &read, &MMU::nopWrite }

#define MMUREGR( name, read ) { &name, #name, &read, &MMU::defaultWrite }

#define MMUREGRm( name, read ) { &name, #name, &read, &MMU::defaultWrite, true }
    
#define MMUREGW( name, write ) { &name, #name, &MMU::defaultRead, &write }

    extern u32 _reserved;
#define MMUREGX() { &MMU::_reserved, "RESERVED", &MMU::readHole, &MMU::writeHole }
#define MMUREGDBG( name ) { &MMU::_reserved, #name, &MMU::dbgRead, &MMU::dbgWrite }
    
#define MMUREG( name ) { &name, #name, &MMU::defaultRead, &MMU::defaultWrite }

    struct Layout {
	u32 base;
	u32 length;
	Register *map;
    };

bool init();
void uninit();

u32 read32(u32 address);
u32 read16(u32 address);
s16 read16s(u32 address);
u8 read8(u32 address);

void write32(u32 address, u32 value);
void write16(u32 address, u16 value);
void write8(u32 address, u8 b);

u32 CPUReadMemory(u32 address);
u32 CPUReadHalfWord(u32 address);
u16 CPUReadHalfWordSigned(u32 address);
u8 CPUReadByte(u32 address);

void CPUWriteMemory(u32 address, u32 value);
void CPUWriteHalfWord(u32 address, u16 value);
void CPUWriteByte(u32 address, u8 b);

} // namespace MMU

