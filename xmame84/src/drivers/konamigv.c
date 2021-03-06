/***************************************************************************

  Konami GV System (aka "Baby Phoenix") - Arcade PSX Hardware
  ===========================================================
  Driver by R. Belmont & smf


Known Dumps
-----------

Game       Description                  Mother Board   Code       Version       Date   Time

pbball96   Powerful Pro Baseball '96    GV999          GV017   JAPAN 1.03   96.05.27  18:00
hyperath   Hyper Athlete                ZV610          GV021   JAPAN 1.00   96.06.09  19:00
susume     Susume! Taisen Puzzle-Dama   ZV610          GV027   JAPAN 1.20   96.03.04  12:00
weddingr   Wedding Rhapsody             ?              GX624   JAA          97.05.29   9:12
simpbowl   Simpsons Bowling             ?              GQ829   UAA          ?


PCB Layouts
-----------

ZV610 PWB301331
|---------------------------------------|
|   000180       056602      LM324   CN8|
|CN2                                    |
|                                       |
|      999A01.7E                     CN6|
|                         CXD2922BQ     |
|      10E                KM416V256BLT-7|
|                                       |
|J     12E                              |
|A CXD2923AR     058239                 |
|M                                      |
|M                     CXD8530BQ        |
|A   D482445LGW-A70            93CF96-2 |
|               CXD8514Q               S|
|    D482445LGW-A70                    C|
|                      67.7376MHz      S|
|         53.693175MHz                 I|
|                                 32MHz |
|    93C46   KM48V514BJ-6  KM48V514BJ-6 |
|            KM48V514BJ-6  KM48V514BJ-6 |
|    CN5      CN3                001231 |
|---------------------------------------|

GV999 PWB301949A
|---------------------------------------|
|                056602      LM324   CN8|
|CN2                                    |
|TEST_SW                                |
|      999A01.7E                     CN6|
|MC44200                  CXD2925Q      |
|      9E                 TC51V4260BJ-80|
|                                       |
|J     12E                              |
|A               058239                 |
|M  53.693175MHz                        |
|M                     CXD8530CQ        |
|A                             93CF96-2 |
|      CXD8561Q                        S|
|              KM4132G271Q-12          C|
|                      67.7376MHz      S|
|         53.693175MHz                 I|
|                                 32MHz |
|    93C46   KM48V514BJ-6  KM48V514BJ-6 |
|            KM48V514BJ-6  KM48V514BJ-6 |
|    CN5      CN3                001231 |
|---------------------------------------|

Notes:
      - Simpsons Bowling uses an unknown board revision with 4 x 16M FLASHROMs & a trackball.

      - 000180 is used for driving the RGB output. It's a very thin piece of very brittle ceramic
        containing a circuit, a LM1203 chip, some smt transistors/caps/resistors etc (let's just say
        placing this thing on the edge of the PCB wasn't a good design choice!)
        On GV999, it has been replaced by three transistors and a MC44200.

      - 056602 seems to be some sort of A/D converter (another ceramic thing containing caps/resistors/transistors and a chip)

      - CXD2922 and CXD2925 are SPU's.

      - The BIOS on ZV610 and GV999 is identical. It's a 4M MASK ROM, compatible with 27C040.

      - The CD contains one MODE 1 data track and several Redbook audio tracks which are streamed to the speaker via CN8.

      - The ZV and GV PCB's are virtually identical aside from some minor component shuffling and the RGB output mechanism.
        However note that the GPU revision is different between the two boards and so are some of the other Sony IC's.

      - CN8 used to connect redbook audio output from CD drive to PCB.

      - CN6 used to connect power to CD drive.

      - CN2 used for extra speaker connection for stereo output.

      - CN3, CN5 used for connecting 3rd and 4th player controls.

      - 001231, 058239 are PALCE16V8H PALs.

      - 10E, 12E are unpopulated positions for 16M TSOP56 FLASHROMs (10E is 9E on GV999).

      - If the CD is swapped to another GV game, the game will boot but will stop with an error '25C MBAD' (the EEPROM is 25C)
        So the games can not be swapped by simply exchanging CDs because the EEPROM will not re-init itself if the CDROM is swapped.
        This appears to be some form of mild protection to stop operators swapping CD's.
        However it is possible to swap games to another PCB by exchanging the CD _AND_ the EEPROM from another PCB which belongs
        to that same game. It won't work with a blank EEPROM or a different games' EEPROM.
*/

#include "driver.h"
#include "state.h"
#include "cdrom.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/eeprom.h"
#include "machine/intelfsh.h"
#include "machine/am53cf96.h"

/* EEPROM handlers */

static NVRAM_HANDLER( konamigv_93C46 )
{
	if( read_or_write )
	{
		EEPROM_save( file );
	}
	else
	{
		EEPROM_init( &eeprom_interface_93C46 );

		if( file )
		{
			EEPROM_load( file );
		}
		else
		{
			EEPROM_set_data( memory_region( REGION_USER2 ), memory_region_length( REGION_USER2 ) );
		}
	}
}

static READ32_HANDLER( eeprom_r )
{
	return 0xffff0000 | ( EEPROM_read_bit() << 13 ) | readinputport( 0 );
}

static WRITE32_HANDLER( eeprom_w )
{
	EEPROM_write_bit((data&0x01) ? 1 : 0);
	EEPROM_set_clock_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data&0x02) ? CLEAR_LINE : ASSERT_LINE);
}

static READ32_HANDLER( inputs_0_r )
{
	return 0xffff0000 | readinputport(1);
}

static READ32_HANDLER( inputs_1_r )
{
	return 0xffff0000 | readinputport(2);
}

static WRITE32_HANDLER( mb89371_w )
{
}

static READ32_HANDLER( mb89371_r )
{
	return 0xffffffff;
}

static ADDRESS_MAP_START( konamigv_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f00001f) AM_READWRITE(am53cf96_r, am53cf96_w)
	AM_RANGE(0x1f100000, 0x1f100003) AM_READ(eeprom_r)
	AM_RANGE(0x1f100004, 0x1f100007) AM_READ(inputs_0_r)
	AM_RANGE(0x1f100008, 0x1f10000b) AM_READ(inputs_1_r)
	AM_RANGE(0x1f180000, 0x1f180003) AM_WRITE(eeprom_w)
	AM_RANGE(0x1f680000, 0x1f68001f) AM_READWRITE(mb89371_r, mb89371_w)
	AM_RANGE(0x1f780000, 0x1f780003) AM_WRITENOP /* watchdog? */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x801fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa01fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

/* SCSI */

static data8_t sector_buffer[ 4096 ];

static void scsi_dma_read( UINT32 n_address, INT32 n_size )
{
	int i;
	int n_this;

	while( n_size > 0 )
	{
		if( n_size > sizeof( sector_buffer ) / 4 )
		{
			n_this = sizeof( sector_buffer ) / 4;
		}
		else
		{
			n_this = n_size;
		}
		if( n_this < 2048 / 4 )
		{
			/* non-READ commands */
			am53cf96_read_data( n_this * 4, sector_buffer );
		}
		else
		{
			/* assume normal 2048 byte data for now */
			am53cf96_read_data( CD_FRAME_SIZE, sector_buffer );
			n_this = 2048 / 4;
		}
		n_size -= n_this;

		i = 0;
		while( n_this > 0 )
		{
			g_p_n_psxram[ n_address / 4 ] =
				( sector_buffer[ i + 0 ] << 0 ) |
				( sector_buffer[ i + 1 ] << 8 ) |
				( sector_buffer[ i + 2 ] << 16 ) |
				( sector_buffer[ i + 3 ] << 24 );
			n_address += 4;
			i += 4;
			n_this--;
		}
	}
}

static void scsi_dma_write( UINT32 n_address, INT32 n_size )
{
	int i;
	int n_this;

	while( n_size > 0 )
	{
		if( n_size > sizeof( sector_buffer ) / 4 )
		{
			n_this = sizeof( sector_buffer ) / 4;
		}
		else
		{
			n_this = n_size;
		}
		n_size -= n_this;

		i = 0;
		while( n_this > 0 )
		{
			sector_buffer[ i + 0 ] = ( g_p_n_psxram[ n_address / 4 ] >> 0 ) & 0xff;
			sector_buffer[ i + 1 ] = ( g_p_n_psxram[ n_address / 4 ] >> 8 ) & 0xff;
			sector_buffer[ i + 2 ] = ( g_p_n_psxram[ n_address / 4 ] >> 16 ) & 0xff;
			sector_buffer[ i + 3 ] = ( g_p_n_psxram[ n_address / 4 ] >> 24 ) & 0xff;
			n_address += 4;
			i += 4;
			n_this--;
		}

		am53cf96_write_data( n_this * 4, sector_buffer );
	}
}

static void scsi_irq(void)
{
	psx_irq_set(0x400);
}

static struct AM53CF96interface scsi_intf =
{
	AM53CF96_DEVICE_CDROM,	/* CD-ROM */
	&scsi_irq,		/* command completion IRQ */
};

static DRIVER_INIT( konamigv )
{
	psx_driver_init();

	/* init the scsi controller and hook up it's DMA */
	am53cf96_init(&scsi_intf);
	psx_dma_install_read_handler(5, scsi_dma_read);
	psx_dma_install_write_handler(5, scsi_dma_write);

	/* also hook up CDDA audio to the CD-ROM drive */
	CDDA_set_cdrom(0, am53cf96_get_device());
}

static MACHINE_INIT( konamigv )
{
	psx_machine_init();
}

static struct PSXSPUinterface konamigv_psxspu_interface =
{
	75
};

static struct CDDAinterface konamigv_cdda_interface =
{
	1,
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT),},
};

static MACHINE_DRIVER_START( konamigv )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( konamigv_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( konamigv )
	MDRV_NVRAM_HANDLER(konamigv_93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, konamigv_psxspu_interface )
	MDRV_SOUND_ADD( CDDA, konamigv_cdda_interface )
MACHINE_DRIVER_END

INPUT_PORTS_START( konamigv )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

INPUT_PORTS_END

/* Simpsons Bowling */

static NVRAM_HANDLER( simpbowl )
{
	nvram_handler_konamigv_93C46( file, read_or_write );
	nvram_handler_intelflash_0( file, read_or_write );
	nvram_handler_intelflash_1( file, read_or_write );
	nvram_handler_intelflash_2( file, read_or_write );
	nvram_handler_intelflash_3( file, read_or_write );
}

static int flash_address;

static READ32_HANDLER( flash_r )
{
	int reg = offset*2;
	int shift = 0;

	if (mem_mask == 0x0000ffff)
	{
		reg++;
		shift = 16;
	}

	if (reg == 4)	/* set odd address */
	{
		flash_address |= 1;
	}

	if (reg == 0)
	{
		int chip = (flash_address >= 0x200000) ? 2 : 0; 
		int ret;

		ret = intelflash_read_byte(chip, flash_address & 0x1fffff) & 0xff;
		ret |= intelflash_read_byte(chip+1, flash_address & 0x1fffff)<<8;
		flash_address++;

		return ret;
	}
	return 0;
}

static WRITE32_HANDLER( flash_w )
{
	int reg = offset*2;
	int chip;

	if (mem_mask == 0x0000ffff)
	{
		reg++;
		data>>= 16;
	}

	switch (reg)
	{
		case 0:
			chip = (flash_address >= 0x200000) ? 2 : 0; 
			intelflash_write_byte(chip, flash_address & 0x1fffff, data&0xff);
			intelflash_write_byte(chip+1, flash_address & 0x1fffff, (data>>8)&0xff);
			break;

		case 1:
			flash_address = 0;
			flash_address |= (data<<1);
			break;
		case 2:
			flash_address &= 0xff00ff;
			flash_address |= (data<<8);
			break;
		case 3:
			flash_address &= 0x00ffff;
			flash_address |= (data<<15);
			break;
	}
}

static data16_t trackball_prev[ 2 ];
static data32_t trackball_data[ 2 ];

static READ32_HANDLER( trackball_r )
{
	if( offset == 0 && mem_mask == 0xffff0000 )
	{
		int axis;
		data16_t diff;
		data16_t value;

		for( axis = 0; axis < 2; axis++ )
		{
			value = readinputport( axis + 3 );
			diff = value - trackball_prev[ axis ];
			trackball_prev[ axis ] = value;
			trackball_data[ axis ] = ( ( diff & 0xf00 ) << 16 ) | ( ( diff & 0xff ) << 8 );
		}
	}
	return trackball_data[ offset ];
}

static READ32_HANDLER( unknown_r )
{
	return 0xffffffff;
}

static MACHINE_INIT( simpbowl )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, flash_r );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f680080, 0x1f68008f, 0, 0, flash_w );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f6800c0, 0x1f6800c7, 0, 0, trackball_r );
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f6800c8, 0x1f6800cb, 0, 0, unknown_r ); /* ?? */

	psx_machine_init();
}

static MACHINE_DRIVER_START( simpbowl )
	MDRV_IMPORT_FROM( konamigv )
	MDRV_MACHINE_INIT( simpbowl )
	MDRV_NVRAM_HANDLER( simpbowl )
MACHINE_DRIVER_END

INPUT_PORTS_START( simpbowl )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START4 )

	/* IN 3 */
	PORT_START
	PORT_ANALOG( 0x7ff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_REVERSE, 100, 63, 0, 0 )

	/* IN 4 */
	PORT_START
	PORT_ANALOG( 0x7ff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 63, 0, 0 )

INPUT_PORTS_END

#define GV_BIOS	\
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )	\
	ROM_LOAD( "999a01.7e",   0x0000000, 0x080000, CRC(ad498d2d) SHA1(02a82a2fe1fba0404517c3602324bfa64e23e478) )

ROM_START( konamigv )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
ROM_END

ROM_START( susume )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "susume.25c",   0x000000, 0x000080, CRC(52f17df7) SHA1(b8ad7787b0692713439d7d9bebfa0c801c806006) )
	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "susume", 0, MD5(6645e48204b32d8619b5ba3169f515ab) SHA1(2e83d3a0b45ed740e2ab27d259347b20b87a3eaf) )
ROM_END

ROM_START( hyperath )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "hyperath.25c", 0x000000, 0x000080, CRC(20a8c435) SHA1(a0f203a999757fba68b391c525ac4b9684a57ba9) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "hyperath", 0, MD5(a777d62bc998768e53d3d764d96cd990) SHA1(dfe0a68258cf33ca09639a752611302b361698e8) )
ROM_END

ROM_START( pbball96 )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "pbball96.25c", 0x000000, 0x000080, CRC(405a7fc9) SHA1(e2d978f49748ba3c4a425188abcd3d272ec23907) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "pbball96", 0, MD5(5962a38e2af6299659f53613956cd9ed) SHA1(a056138fc68bd1580b1de89b622b159150413a3f) )
ROM_END

ROM_START( weddingr )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "weddingr.25c", 0x000000, 0x000080, CRC(b90509a0) SHA1(41510a0ceded81dcb26a70eba97636d38d3742c3) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "weddingr", 0, MD5(cacc28156b037e13098f3624ae92ab85) SHA1(e6481c367ad24aa285e51c01221a169b1b74b15f) )
ROM_END

ROM_START( simpbowl )
	GV_BIOS

	ROM_REGION( 0x0000080, REGION_USER2, 0 ) /* default eeprom */
	ROM_LOAD( "simpbowl.25c", 0x000000, 0x000080, CRC(2c61050c) SHA1(16ae7f81cbe841c429c5c7326cf83e87db1782bf) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "simpbowl", 0, MD5(47702fc060f3f1fbb2dba84ea3544a4a) SHA1(791ce11b0645fd5c3f5b30483bad879f26bb97db) )
ROM_END

/* BIOS placeholder */
GAMEX( 1995, konamigv, 0, konamigv, konamigv, konamigv, ROT0, "Konami", "Baby Phoenix/GV System", NOT_A_DRIVER )

GAMEX( 1996, pbball96, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Powerful Baseball '96 (GV017 JAPAN 1.03)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, hyperath, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Hyper Athlete (GV021 JAPAN 1.00)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, susume,   konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Susume! Taisen Puzzle-Dama (GV027 JAPAN 1.20)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, weddingr, konamigv, konamigv, konamigv, konamigv, ROT0, "Konami", "Wedding Rhapsody (GX624 JAA)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 2000, simpbowl, konamigv, simpbowl, simpbowl, konamigv, ROT0, "Konami", "Simpsons Bowling (GQ829 UAA)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
