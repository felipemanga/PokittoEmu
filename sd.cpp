#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>

#include "types.hpp"
#include "sd.hpp"
#include "spi.hpp"

extern u32 verbose;

namespace SD {
    u32 length;
    std::unique_ptr<u8[]> image;

    bool enabled = false;
    bool checkCRC = true;
    u32 writeAddress, writeCount;
    u32 nextReadAddress;
    u32 command = 0;
    u32 altCommand = false;
    u32 argument = 0;
    u32 state = 0;
    bool idle = true;
    u32 resetCounter = 0;
    std::vector<u8> response;
    std::string outName;
    
    bool init( const std::string &fileName, const std::string &outName ){

	using std::ios;

        SD::outName = outName;
	atexit([](){
                   if(SD::outName.empty() || !image )
                       return;
                   std::ofstream os(SD::outName.c_str(), ios::binary);
                   os.write(reinterpret_cast<char *>(&image[0]), length );
               });


        if( fileName.empty() ){
            length = 1024;
            image = std::make_unique<u8[]>( length );
            return true;
        }

	std::ifstream is( fileName.c_str(), ios::binary );
	
	if( !is.is_open() )
	    return false;

	is.seekg( 0, ios::end );
	length = is.tellg();
	image = std::make_unique<u8[]>( length );
	
	is.seekg( 0, ios::beg );
	is.read( reinterpret_cast<char *>(&image[0]), length );

	return true;

    }

    u16 crc16[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    };

    using Command = void (*) ( u32 );

    void stubCommand( u32 arg ){
	
	std::cout << "Unknown SD command "
		  << std::dec
		  << command
		  << " arg: "
		  << std::hex
		  << arg
		  << std::endl;
	
	response = std::vector<u8>{4};
    }
    
    Command commands[] = {
	[]( u32 arg ){
	    idle = true;
	    response = std::vector<u8>{1};
	},
	
	stubCommand, // 1
	stubCommand, // 2
	stubCommand, // 3
	stubCommand, // 4
	stubCommand, // 5
	stubCommand, // 6
	stubCommand, // 7
	[]( u32 arg ){ // detect SDv2
	    // arg:  Rsv(0)[31:12], Supply Voltage(1)[11:8], Check Pattern(0xAA)[7:0]
	    response = std::vector<u8>{u8(idle?1:0), 0, 0, 0x01, 0xAA};
	},
	
	stubCommand, // 9
	stubCommand, // 10
	stubCommand, // 11
	
	[]( u32 arg ){ // 12

	    if( state == 16 )
		state = 0;
	    
	    response = std::vector<u8>{ 0, 0 };
	    
	},

	[]( u32 arg ){ // verify that the programming was successful
	    response = std::vector<u8>{ 0, 0 };
	},

	stubCommand, // 14
	stubCommand, // 15
	[]( u32 arg ){
	    response = std::vector<u8>{ u8( arg == 512 ? (idle?1:0) : 1<<6 ) };
	},
	
	[]( u32 arg ){ // 17 - single block read
	    if( verbose > 1 )
		std::cout << "Read block" << arg << std::endl;
	    u32 addr = arg*512;
	    u16 CRC=0;
	    response.clear();
	    response.push_back(0); // R1
	    response.push_back(~1); // Data Token For 17
	    for( u32 i=0; i<512; ++i ){
		u8 ret = image[addr++];
		CRC = ((CRC << 8) ^ crc16[((CRC >> 8) ^ ret) & 0x00FF]);
		response.push_back(ret);
	    }
	    response.push_back( CRC>>8 );
	    response.push_back( CRC&0xFF );
	},

	[]( u32 arg ){ // 18 - multi block read
	    if( verbose > 1 )
		std::cout << "Read multi-block " << arg << std::endl;
	    u32 addr = arg*512;
	    nextReadAddress = arg+1;
	    state = 16;
	    u16 CRC=0;
	    response.clear();
	    response.push_back(0); // R1
	    response.push_back(~1); // Data Token For 17
	    for( u32 i=0; i<512; ++i ){
		u8 ret = image[addr++];
		CRC = ((CRC << 8) ^ crc16[((CRC >> 8) ^ ret) & 0x00FF]);
		response.push_back(ret);
	    }
	    response.push_back( CRC>>8 );
	    response.push_back( CRC&0xFF );
	},

	stubCommand, // 19
	stubCommand, // 20
	stubCommand, // 21
	stubCommand, // 22
	stubCommand, // 23
	[]( u32 arg ){ // write block
	// std::cout << ("Writing to", arg);
	    response = std::vector<u8>{0};
	    writeAddress = arg * 512;
	    state = 1;
	},
	
	stubCommand, // 25
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 30
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 35
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 40
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 45
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 50
	stubCommand, stubCommand, stubCommand, stubCommand, // 54
	[]( u32 arg ){
	    altCommand = true;
	    response = std::vector<u8>{u8(idle?1:0)};
	},
	
	stubCommand, // 56
	stubCommand, // 57
	[]( u32 arg ){ // check OCR bit
	    response = std::vector<u8>{u8(idle?1:0),0x40,0x10,0,0};
	},
	[]( u32 arg ){ // enable/disable CRC checking
	    checkCRC = !!arg;
	    response = std::vector<u8>{u8(idle?1:0)};
	},
	stubCommand, // 60
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 65
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 70
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 75
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 80
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 85
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 90
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, // 95
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //100
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //105
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //110
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //115
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //120
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //125
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //130
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //135
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //140
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //145
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //150
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //155
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //160
	stubCommand, stubCommand, stubCommand, stubCommand, stubCommand, //165
	stubCommand, stubCommand, stubCommand, //168
	
	[]( u32 arg ){
	    //arg: Rsv(0)[31], HCS[30], Rsv(0)[29:0]
	    idle = false;
	    response = std::vector<u8>{0};
	},

	[]( u32 arg ){ // pullup resistor? Never heard of it.
	    response = std::vector<u8>{0};
	}
    };

    void write( u32 b ){

	if( !enabled && b == 0xFF ){
	    resetCounter += 8;
	    if( resetCounter >= 74 ){
		state = 1;
		checkCRC = true;
		writeAddress = ~0;
	    }
            response.clear();
	    SPI::spi0In( 0xFF, true );
	    return;
	}

	if( !enabled ) return;

	resetCounter = 0;

	if( state == 16 ){ // multi-block read
	    if( (b&0x3F) == 12 ){
		response = std::vector<u8>({0, 0, 0, 0, 0, 0, 0, 0, 0});
		state = 1;
	    }else if( response.empty() ){
		commands[18]( nextReadAddress );
	    }
	}
	
	if( response.size() ){
	    SPI::spi0In( response[0], true );
	    response.erase( response.begin() );
	    return;
	}

	switch( state ){
	case 1:
	    if( b == 0xFF ) break;
	    else if( writeAddress != ~0 ){
			
		if( b == 0xFE ){
		    writeCount = 512; // write start
		    state = 12;
		    SPI::spi0In( 0 );
		    return;
		}
		if( b == 0xFD ){
		    state = 1; // write terminator
		    SPI::spi0In( 0 );
		    response = std::vector<u8>{0};
		    return;
		}
			
	    }
		    
	    if( b > 0x7F )
		break;

	    command = b & 0x3F; // 0b111111;
	    argument = 0;
	    state++;
	    break;
		    
	case 2:
	case 3:
	case 4:
	case 5:
	    argument = (argument<<8) | b;
	    state++;
	    break;
		    
	case 6: // CRC
	    state++;
	    break;
		    
	case 7:
	    state = 1;

	    if( verbose >= 1 )
		std::cout << "SDCMD"
		      << (altCommand ? "A" : "")
		      << "CMD"
		      << std::to_string(command)
		      << std::endl;
		    
	    if( altCommand )
		command += 0x80;
	    altCommand = false;
		    
	    Command commandFunc;
	    if( command >= (sizeof(commands)/sizeof(commands[0])) )
		commandFunc = stubCommand;
	    else
		commandFunc = commands[ command ];
		    
	    commandFunc( argument );
		    
	    SPI::spi0In( response[0], true );
	    response.erase( response.begin() );
		    
	    return;

	case 10: // write block
	case 11:
	case 13: // crc1
	case 14: // crc2
	    state++;
	    SPI::spi0In( 0, true );
	    break;
	case 12:
	    image[ writeAddress++ ] = b;
	    writeCount--;
	    if( !writeCount )
		state++;
	    SPI::spi0In( 0, true );
	    return;
	case 15:
	    SPI::spi0In( 0x5, true );
	    state = 1;
	    return;

	}
		
	SPI::spi0In( 0xFF, true );
		
    }
    
}

