// TODO:

// set song loop point correctly
// FINETUNE!!!
// tempo?? speed??
// reset function

// change sample_lengths[] so it already has the correct 16bit value for mega65

#include <stdint.h>
#include "macros.h"
#include "registers.h"
#include "dma.h"
#include "modplay.h"

// #pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

dma_copyjob mp_dmacopyjob;

#define MP_MAX_INSTRUMENTS					32
#define MP_NUMRASTERS						(uint32_t)(2 * 312)	// PAL 0-311, NTSC 0-262
#define MP_RASTERS_PER_SECOND				(uint32_t)(MP_NUMRASTERS * 50)
#define MP_RASTERS_PER_MINUTE				(uint32_t)(MP_RASTERS_PER_SECOND * 60)
#define MP_NUM_SIGS							4

#define MP_SAMPLE_RATE_DIVISORLO			0x8B
#define MP_SAMPLE_RATE_DIVISORMID			0x67
#define MP_SAMPLE_RATE_DIVISORHI			0x16

int8_t mp_modsigs[MP_NUM_SIGS][4] =
{
	{ 0x4d, 0x2e, 0x4b, 0x2e },	// M.K.
	{ 0x4d, 0x21, 0x4b, 0x21 }, // M!K!
	{ 0x46, 0x4c, 0x54, 0x34 }, // FLT4
	{ 0x46, 0x4c, 0x54, 0x38 }  // FLT8
};

int16_t	mp_sine[64] =
{
	   0,   24,   49,   74,   97,  120,  141,  161,  180,  197,  212,  224,  235,  244,  250,  253,
	 254,  253,  250,  244,  235,  224,  212,  197,  180,  161,  141,  120,   97,   74,   49,   24,
	   0,  -24,  -49,  -74,  -97, -120, -141, -161, -180, -197, -212, -224, -235, -244, -250, -253,
	-254, -253, -250, -244, -235, -224, -212, -197, -180, -161, -141, -120,  -97,  -74,  -49,  -24,
};

int16_t	mp_saw[64] =
{
	 255,  247,  239,  231,  223,  215,  207,  199,  191,  183,  175,  167,  159,  151,  143,  135,
	 127,  119,  111,  103,   95,   87,   79,   71,   63,   55,   47,   39,   31,   23,   15,    7,
	  -1,   -9,  -17,  -25,  -33,  -41,  -49,  -57,  -65,  -73,  -81,  -89,  -97, -105, -113, -121,
	-129, -137, -145, -153, -161, -169, -177, -185, -193, -201, -209, -217, -225, -233, -241, -249,
};

int16_t	mp_square[64] =
{
	 255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
	 255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,
	-256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
	-256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256,
};

int16_t* mp_waves[3] = { mp_sine, mp_saw, mp_square };

uint8_t mp_enabled_channels[4] = { 1, 1, 1, 1 };

// GLOBAL DATA

uint8_t			mp_playing;

uint8_t			mp_done						= 1;
uint8_t			mp_patternset;
uint8_t			mp_row;
uint8_t			mp_currow;
uint8_t			mp_pattern;
uint8_t			mp_delcount;
uint8_t			mp_globaltick;
uint8_t			mp_delset;
uint8_t			mp_inrepeat;
uint8_t			mp_addflag;
uint8_t			mp_arpeggiocounter;

uint8_t			mp_nextspeed;
uint8_t			mp_nexttempo;
uint8_t			mp_loop						= 1;
uint8_t			mp_bpm						= 125;
uint8_t			mp_curpattern;
uint8_t			mp_song_loop_point;			// unused

uint8_t			freqlo;
uint8_t			freqhi;
uint32_t		sample_adr;
uint16_t		sample_end_addr;
uint8_t			sample_address0;
uint8_t			sample_address1;
uint8_t			sample_address2;

// MOD DATA FOR 1 MOD ----------------------------------------

uint32_t		mod_attic_addr;
uint8_t			mod_sigsize;					// size of signature (0 or 4)
uint8_t			mod_numinstruments;
uint32_t		mod_sample_addr;
uint32_t		mod_sample_offset;
uint32_t		mod_sample_data;
uint32_t		mod_patternlist_offset;
uint32_t		mod_patterns_offset;
uint32_t		mod_patterns_data;
uint32_t		mod_patternlist_data;
uint8_t			mod_numpatterns;
uint8_t			mod_songlength;
uint8_t			mod_speed;
uint16_t		mod_tempo;
uint8_t			mod_patternlist[128];
uint8_t			mod_tmpbuf[23];
uint8_t			mp_currowdata[16];				// data for current pattern

// SAMPLE DATA FOR ALL INSTRUMENTS ---------------------------

uint16_t		sample_lengths				[MP_MAX_INSTRUMENTS];
int8_t			sample_finetune				[MP_MAX_INSTRUMENTS];
uint8_t			sample_vol					[MP_MAX_INSTRUMENTS];
uint16_t		sample_repeatpoint			[MP_MAX_INSTRUMENTS];
uint16_t		sample_repeatlength			[MP_MAX_INSTRUMENTS];
uint32_t		sample_addr					[MP_MAX_INSTRUMENTS];

// CHANNEL DATA FOR ALL 4 CHANNELS ---------------------------

uint8_t			channel_sample				[4];
int8_t			channel_volume				[4];
int8_t			channel_tempvolume			[4];
uint32_t		channel_index				[4];			// used in 'retrigger note + x vblanks (ticks)', depends on offset
uint8_t			channel_repeat				[4];			// was bool, now 0-1 unsigned char
uint8_t			channel_stop				[4];			// was bool, now 0-1 unsigned char
uint8_t			channel_deltick				[4];
uint16_t		channel_period				[4];
uint16_t		channel_arp[3]				[4];
uint16_t		channel_portdest			[4];
uint16_t		channel_tempperiod			[4];
uint8_t			channel_portstep			[4];
uint8_t			channel_cut					[4];
int8_t			channel_retrig				[4];			// seems to be always 0 and never set again
uint8_t			channel_vibspeed			[4];
uint8_t			channel_vibwave				[4];
uint8_t			channel_vibpos				[4];
uint8_t			channel_vibdepth			[4];
uint8_t			channel_tremspeed			[4];
uint8_t			channel_tremwave			[4];
uint8_t			channel_trempos				[4];
uint8_t			channel_tremdepth			[4];
int8_t			channel_looppoint			[4];
int8_t			channel_loopcount			[4];
uint16_t		channel_offset				[4];
uint16_t		channel_offsetmem			[4];

// ------------------------------------------------------------------------------------

uint8_t			mp_emptysample				= 0;

// ------------------------------------------------------------------------------------

uint16_t mp_periods[36] =
{
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
};

// ------------------------------------------------------------------------------------

void modplay_enable()
{
	mp_playing = 1;
}

void modplay_mute()
{
	// Mute all channels
	poke(0xd729, 0);
	poke(0xd739, 0);
	poke(0xd749, 0);
	poke(0xd759, 0);
	poke(0xd71c, 0);
	poke(0xd71d, 0);
	poke(0xd71e, 0);
	poke(0xd71f, 0);

	// disable audio dma
	AUDIO_DMA.AUDEN		= 0b00000000;

	// Stop all DMA audio first
	AUDIO_DMA.CHANNELS[0].CONTROL = 0;
	AUDIO_DMA.CHANNELS[1].CONTROL = 0;
	AUDIO_DMA.CHANNELS[2].CONTROL = 0;
	AUDIO_DMA.CHANNELS[3].CONTROL = 0;
}

void modplay_disable()
{
	mp_playing = 0;

	// set sample address to the empty sample
	sample_address0 = ((uint32_t)(&mp_emptysample) >>  0) & 0xff;
	sample_address1 = ((uint32_t)(&mp_emptysample) >>  8) & 0xff;
	sample_address2 = ((uint32_t)(&mp_emptysample) >> 16) & 0xff;

	for(int i = 0; i < 4; i++)
	{
		poke(0xd72a + i*16, sample_address0);
		poke(0xd72b + i*16, sample_address1);
		poke(0xd72c + i*16, sample_address2);
	}
}

void mp_dmacopy(uint32_t source_address, uint32_t destination_address, uint16_t count)
{
	mp_dmacopyjob.count					= count;
	mp_dmacopyjob.source_addr			= (source_address           ) & 0xffff;
	mp_dmacopyjob.source_bank_and_flags	= (source_address      >> 16) & 0x7f;
	mp_dmacopyjob.dest_addr				= (destination_address      ) & 0xffff;
	mp_dmacopyjob.dest_bank_and_flags	= (destination_address >> 16) & 0x7f;

	DMA.EN018B				= 0x01;
	DMA.ADDRBANK			= 0x00;
	DMA.ADDRMSB				= (((uint16_t)&mp_dmacopyjob) >> 8);
	DMA.ETRIG				= (((uint16_t)&mp_dmacopyjob) & 0xff);
}

// ------------------------------------------------------------------------------------

int8_t mp_findperiod(uint16_t period)
{
	// this takes 6 steps at most to reach final period (36/2/2/2/2/2/2)
	if(period > 856 || period < 113)
		return -1;

	uint8_t upper = 35;
	uint8_t lower = 0;
	uint8_t mid;

	while(upper >= lower)
	{
		mid = (upper + lower) / 2;
		if(mp_periods[mid] == period)
			return mid;
		else if(mp_periods[mid] > period)
			lower = mid + 1;
		else
			upper = mid - 1;
	}

	return -1;
}

// ------------------------------------------------------------------------------------

void mp_preprocesseffects(uint8_t* data)
{
	if ((*(data+2) & 0x0f) == 0x0f) // set speed / tempo
	{
		uint8_t effectdata = *(data + 3);

		if(effectdata > 0x1f)
		{
			mod_tempo		= effectdata;
			mp_nexttempo	= effectdata;
		}
		else
		{
			mod_speed = effectdata;
			mp_nextspeed = effectdata;
		}
	}
}

// ------------------------------------------------------------------------------------

void mp_processnote(uint8_t channel, uint8_t *data)
{
	uint8_t tempsam = ((((*data)) & 0xf0) | ((*(data + 2) >> 4) & 0x0f));

	uint16_t period = (((uint16_t)((*data) & 0x0f)) << 8) | (uint16_t)(*(data + 1));

	uint8_t effectdata = *(data + 3);
	uint8_t tempeffect = *(data + 2) & 0x0f;

	// EDx : delay note x vblanks
	if(mp_globaltick == 0 && tempeffect == 0x0e && (effectdata & 0xf0) == 0xd0)
		channel_deltick[channel] = (effectdata & 0x0f) % mod_speed;

	uint8_t ch_ofs = channel << 4;

	uint8_t triggersample = 0; // NOT IN ORIGINAL SOURCE - FIND BETTER WAY OF HANDLING THIS!!!

	if(mp_globaltick == channel_deltick[channel]) // 0 if no note delay
	{
		if((period || tempsam) && !mp_inrepeat)
		{
			triggersample = 1;

			if(tempsam)
			{
				channel_stop[channel] = 0;
				tempsam--;
				if(tempeffect != 0x03 && tempeffect != 0x05)
					channel_offset[channel] = 0;
				channel_sample[channel] = tempsam;
				channel_volume[channel] = sample_vol[tempsam];
				channel_tempvolume[channel] = channel_volume[channel];
			}

			if(period)
			{
				if(tempeffect != 0x03 && tempeffect != 0x05)
				{
					if(tempeffect == 0x09)
					{
						if(effectdata)
							channel_offsetmem[channel] = effectdata * 0x100;
						channel_offset[channel] += channel_offsetmem[channel];
					}
					channel_period    [channel]	= period;
					channel_tempperiod[channel]	= period;
					channel_index     [channel] = channel_offset[channel];
					channel_stop      [channel]	= 0;
					channel_repeat    [channel]	= 0;
					channel_vibpos    [channel]	= 0;
					channel_trempos   [channel]	= 0;
				}
				channel_portdest[channel] = period;
			}
		}

		switch (tempeffect)
		{
			case 0x00: // Normal play or Arpeggio
				if(effectdata)
				{
					channel_period[channel] = channel_portdest[channel];
					int8_t base = mp_findperiod(channel_period[channel]);

					if(base == -1)
					{
						channel_arp[0][channel] = channel_period[channel];
						channel_arp[1][channel] = channel_period[channel];
						channel_arp[2][channel] = channel_period[channel];
						break;
					}

					mp_arpeggiocounter = 0;

					uint8_t step1 = base + ((effectdata >> 4) & 0x0f);
					uint8_t step2 = base + ((effectdata     ) & 0x0f);

					channel_arp[0][channel] = channel_period[channel];

					if(step1 > 35)
					{
						if(step1 == 36)
							channel_arp[1][channel] = 0;
						else
							channel_arp[1][channel] = mp_periods[(step1 - 1) % 36];
					}
					else
					{
						channel_arp[1][channel] = mp_periods[step1];
					}

					if(step2 > 35)
					{
						if(step2 == 36)
							channel_arp[2][channel] = 0;
						else
							channel_arp[2][channel] = mp_periods[(step2 - 1) % 36];
					}
					else channel_arp[2][channel] = mp_periods[step2];
				}

				break;

			case 0x03: // Tone Portamento
				if(effectdata)
					channel_portstep[channel] = effectdata;
				break;

			case 0x04: // vibrato
				if(effectdata & 0x0f)
					channel_vibdepth[channel] = (effectdata     ) & 0x0f;
				if(effectdata & 0xf0)
					channel_vibspeed[channel] = (effectdata >> 4) & 0x0f;
				break;

			case 0x07: // tremolo
				if(effectdata)
				{
					channel_tremdepth[channel] = (effectdata     ) & 0x0f;
					channel_tremspeed[channel] = (effectdata >> 4) & 0x0f;
				}
				break;

			case 0x0b: // position jump
				if(mp_currow == mp_row)
					mp_row = 0;
				mp_pattern = effectdata;
				mp_patternset = 1;
				break;

			case 0xc: // set volume
				if(effectdata > 63)
					channel_volume[channel] = 63;
				else
					channel_volume[channel] = effectdata;
				channel_tempvolume[channel] = channel_volume[channel];
				break;

			case 0x0d: // mp_row jump
				if(mp_delcount)
					break;
				if(!mp_patternset)
					mp_pattern++;
				if(mp_pattern >= mod_songlength)
				{
					if(!mp_loop)
						mp_done = 1;
					mp_pattern = 0;
				}
				mp_row = (effectdata >> 4) * 10 + (effectdata & 0x0f);
				mp_patternset = 1;
				if(mp_addflag)
					mp_row++; // emulate protracker EEx + Dxx bug
				break;

			case 0x0e:
			{
				switch(effectdata & 0xf0)
				{
					case 0x10:
						channel_period[channel] -= effectdata & 0x0f;
						channel_tempperiod[channel] = channel_period[channel];
						break;

					case 0x20:
						channel_period[channel] += effectdata & 0x0f;
						channel_tempperiod[channel] = channel_period[channel];
						break;

					case 0x60: // jump to loop, play x times
						if(!(effectdata & 0x0f))
						{
							channel_looppoint[channel] = mp_row;
						}
						else if(effectdata & 0x0f)
						{
							if(channel_loopcount[channel] == -1)
							{
								channel_loopcount[channel] = (effectdata & 0x0f);
								mp_row = channel_looppoint[channel];
							}
							else if(channel_loopcount[channel])
							{
								mp_row = channel_looppoint[channel];
							}
							channel_loopcount[channel]--;
						}
						break;

					case 0xa0:
						channel_volume[channel] += (effectdata & 0x0f);
						channel_tempvolume[channel] = channel_volume[channel];
						break;

					case 0xb0:
						channel_volume[channel] -= (effectdata & 0x0f);
						channel_tempvolume[channel] = channel_volume[channel];
						break;

					case 0xc0:
						channel_cut[channel] = effectdata & 0x0f;
						break;

					case 0xe0: // delay pattern x notes
						if(!mp_delset)
							mp_delcount = effectdata & 0x0f;
						mp_delset = 1;
						mp_addflag = 1; // emulate bug that causes protracker to cause Dxx to jump too far when used in conjunction with EEx
						break;

					case 0xf0:
						// c->funkspeed = funktable[effectdata & 0x0f];
						break;

					default:
						break;
				}
			}

			default:
				break;
		}

		if(channel_tempperiod[channel] == 0 /* || c->sample[channel] == NULL || c->sample->length == 0 */)
			channel_stop[channel] = 1;
	}
	else if(channel_deltick[channel] == 0)
	{
		switch (tempeffect)
		{
			case 0x00: // normal / arpeggio
				if(effectdata)
					channel_tempperiod[channel] = channel_arp[mp_arpeggiocounter % 3][channel];
				break;

			case 0x01: // slide up
				channel_period[channel] -= effectdata;
				channel_tempperiod[channel] = channel_period[channel];
				break;

			case 0x02: // slide down
				channel_period[channel] += effectdata;
				channel_tempperiod[channel] = channel_period[channel];
				break;

			case 0x05: // tone portamento + volume slide
				if(effectdata & 0xf0)
					channel_volume[channel] += ((effectdata >> 4) & 0x0f); // slide up
				else
					channel_volume[channel] -= effectdata; // slide down
				channel_tempvolume[channel] = channel_volume[channel];
				// no break, exploit fallthrough

			case 0x03: // tone portamento
				if(channel_portdest[channel] > channel_period[channel])
				{
					channel_period[channel] += channel_portstep[channel];
					if(channel_period[channel] > channel_portdest[channel])
						channel_period[channel] = channel_portdest[channel];
				}
				else if(channel_portdest[channel] < channel_period[channel])
				{
					channel_period[channel] -= channel_portstep[channel];
					if(channel_period[channel] < channel_portdest[channel])
						channel_period[channel] = channel_portdest[channel];
				}
				channel_tempperiod[channel] = channel_period[channel];
				break;

			case 0x06: // vibrato + volume slide
				if(effectdata & 0xf0)
					channel_volume[channel] += ((effectdata >> 4) & 0x0f); //slide up
				else
					channel_volume[channel] -= effectdata; // slide down
				channel_tempvolume[channel] = channel_volume[channel];
				// no break, exploit fallthrough

			case 0x04: // vibrato
				channel_tempperiod[channel]  = channel_period[channel] + ((channel_vibdepth[channel] * mp_waves[channel_vibwave[channel] & 3][channel_vibpos[channel]]) >> 7);
				channel_vibpos[channel]     += channel_vibspeed[channel];
				channel_vibpos[channel]     %= 64;
				break;

			case 0x07: // tremolo
				channel_tempvolume[channel]  = channel_volume[channel] + ((channel_tremdepth[channel] * mp_waves[channel_tremwave[channel] & 3][channel_trempos[channel]]) >> 6);
				channel_trempos[channel]    += channel_tremspeed[channel];
				channel_trempos[channel]    %= 64;
				break;

			case 0x0a: // volume slide
				if(effectdata & 0xf0)
					channel_volume[channel] += ((effectdata>>4) & 0x0f); // slide up
				else
					channel_volume[channel] -= effectdata; // slide down
				channel_tempvolume[channel] = channel_volume[channel];
				break;

			case 0x0e: // E commands
			{
				switch(effectdata & 0xf0)
				{
					case 0x00: // set filter (0 on, 1 off)
						break;

					case 0x30: // glissando control (0 off, 1 on, use with tone portamento)
						break;

					case 0x40: // set vibrato waveform (0 sine, 1 ramp down, 2 square)
						channel_vibwave[channel] = effectdata & 0x0f;
						break;

					case 0x50: // set finetune // MODSPEC SAYS THIS IS 'SET LOOP POINT'???
					{
						int8_t tempfinetune = effectdata & 0x0f;
						if(tempfinetune > 0x07)
							tempfinetune |= 0xf0;
						sample_finetune[channel_sample[channel]] = tempfinetune;
						break;
					}

					case 0x70: // set tremolo waveform (0 sine, 1 ramp down, 2 square)
						channel_tremwave[channel] = effectdata & 0x0f;
						break;

					// No effect 0x0E80

					case 0x90: // retrigger note + x vblanks (ticks)
						if(((effectdata & 0x0f) == 0) || (mp_globaltick % (effectdata & 0x0f)) == 0)
							channel_index[channel] = channel_offset[channel];
						break;

					case 0xc0: // cut from note + x vblanks
						if(mp_globaltick == (effectdata & 0x0f))
							channel_volume[channel] = 0;
						break;
				}
				break;
			}

			case 0xf: // Speed / tempo
				if(effectdata == 0)
				{
					mp_done = 1;
					break;
				}
				if (effectdata < 0x20) // speed (00-1F) / ticks per row (normally 6)
				{
					mod_speed = (effectdata & 0x1f);
				}
				else // tempo (20-FF)
				{
					//beats_per_minute = effectdata;
					// // mod_tempo *= 6 / (effectdata & 0x1f);
					//mod_tempo = MP_RASTERS_PER_MINUTE / beats_per_minute / ROWS_PER_BEAT;
					//mod_speed = mod_tempo / NUMRASTERS;
				}
				break;
		}
	}

	if     (channel_volume[channel] < 0)		channel_volume[channel] = 0;
	else if(channel_volume[channel] > 63)		channel_volume[channel] = 63;

	if     (channel_tempvolume[channel] < 0)	channel_tempvolume[channel] = 0;
	else if(channel_tempvolume[channel] > 63)	channel_tempvolume[channel] = 63;
	
	if     (channel_tempperiod[channel] > 856)	channel_tempperiod[channel] = 856;
	else if(channel_tempperiod[channel] < 113)	channel_tempperiod[channel] = 113;

	if(mp_globaltick == mod_speed - 1)
	{
		channel_tempperiod[channel] = channel_period[channel];
		channel_deltick[channel] = 0;
	}

	uint8_t curchansamp = channel_sample[channel];

	// SET FREQUENCY

	MATH.MULTINA0 = 0xff;																// calculate frequency // freq = 0xFFFFL / period
	MATH.MULTINA1 = 0xff;
	MATH.MULTINA2 = 0;
	MATH.MULTINA3 = 0;

	uint16_t finetunedperiod = channel_tempperiod[channel];
	
	//if(sample_finetune[curchansamp] != 0)
	//	finetunedperiod -= 8 * sample_finetune[curchansamp];

	MATH.MULTINB0 = (finetunedperiod >> 0) & 0xff;
	MATH.MULTINB1 = (finetunedperiod >> 8) & 0xff;
	MATH.MULTINB2 = 0;
	MATH.MULTINB3 = 0;

	// wait 20 cycles at most for DIV calculation to finish
	__asm(
		" lda 0xd020\n"
		" sta 0xd020\n"
		" lda 0xd020\n"
		" sta 0xd020\n"
		" sta 0xd020"
	);

	freqlo = MATH.DIVOUTFRACT0;
	freqhi = MATH.DIVOUTFRACT1;

	MATH.MULTINA0 = MP_SAMPLE_RATE_DIVISORLO;
	MATH.MULTINA1 = MP_SAMPLE_RATE_DIVISORMID;
	MATH.MULTINA2 = MP_SAMPLE_RATE_DIVISORHI;
	MATH.MULTINA3 = 0;

	MATH.MULTINB0 = freqlo;
	MATH.MULTINB1 = freqhi;
	MATH.MULTINB2 = 0;
	MATH.MULTINB3 = 0;

	poke(0xd724 + ch_ofs, MATH.MULTOUT2);													// Pick results from output / 2^16
	poke(0xd725 + ch_ofs, MATH.MULTOUT3);
	poke(0xd726 + ch_ofs, 0);

	// SET VOLUME

	if(channel_stop[channel])
	{
		poke(0xd729 + ch_ofs,  0);															// CH0VOLUME
		poke(0xd71c + channel, 0);															// CH0RVOL
	}
	else
	{
		poke(0xd729 + ch_ofs,  channel_tempvolume[channel]);								// CH0VOLUME
		poke(0xd71c + channel, channel_tempvolume[channel] >> 1);							// CH0RVOL
	}

	// TRIGGER SAMPLE

	if(mp_globaltick == 0 && triggersample == 1 && !channel_stop[channel])
	{
		if(mp_enabled_channels[channel] == 0)
			return;

		poke(0xd720 + ch_ofs, 0x00);														// Stop playback while loading new sample data

		sample_adr = sample_addr[curchansamp] + channel_offset[channel];
		sample_address0 = (sample_adr >>  0) & 0xff;
		sample_address1 = (sample_adr >>  8) & 0xff;
		sample_address2 = (sample_adr >> 16) & 0xff;

		poke(0xd72a + ch_ofs, sample_address0);												// Load sample address into current addr to set start address for playback
		poke(0xd72b + ch_ofs, sample_address1);
		poke(0xd72c + ch_ofs, sample_address2);

		if(sample_repeatpoint[curchansamp])
		{
			sample_adr = (uint32_t)sample_addr[curchansamp] + 2 * sample_repeatpoint[curchansamp];
			sample_address0 = (sample_adr >>  0) & 0xff;
			sample_address1 = (sample_adr >>  8) & 0xff;
			sample_address2 = (sample_adr >> 16) & 0xff;
			poke(0xd721 + ch_ofs, sample_address0);											// set repeat point for repeating sample
			poke(0xd722 + ch_ofs, sample_address1);
			poke(0xd723 + ch_ofs, sample_address2);

			sample_end_addr = sample_adr + 2 * sample_repeatlength[curchansamp];			// Sample loop end address
			sample_address0 = (sample_end_addr >>  0) & 0xff;
			sample_address1 = (sample_end_addr >>  8) & 0xff;
			poke(0xd727 + ch_ofs, sample_address0);
			poke(0xd728 + ch_ofs, sample_address1);

			poke(0xd720 + ch_ofs, 0b11000010);												// Enable playback +   looping of channel 0, 8-bit, no unsigned samples
		}
		else
		{
			sample_end_addr = sample_addr[curchansamp] + sample_lengths[curchansamp];		// Sample end address
			sample_address0 = (sample_end_addr >>  0) & 0xff;
			sample_address1 = (sample_end_addr >>  8) & 0xff;
			poke(0xd727 + ch_ofs, sample_address0);
			poke(0xd728 + ch_ofs, sample_address1);

			poke(0xd720 + ch_ofs, 0b10000010);												// Enable playback + nolooping of channel 0, 8-bit, no unsigned samples
		}

		poke(0xd711, 0b10010000);															// Enable audio dma, enable bypass of audio mixer
	}
}

// ------------------------------------------------------------------------------------

void modplay_play()
{
	if(!mp_playing || mp_done)
		return;

	poke(0xd020, peek(0xd020)+1);

	// setup some DMA stuff that stays the same during modplay_play
	DMA.ADDRBANK		= 0;
	DMA.ADDRMB			= 0;

	mp_arpeggiocounter++;

	if(mp_row == 64)
	{
		mp_row = 0;
		mp_pattern++;
	}

	if(mp_pattern >= mod_songlength)
	{
		if(mp_loop)
		{
			mp_done			=    0;
			mp_pattern		=    0;
			mp_row			=    0;
			mp_globaltick	=    0;
			mod_speed		=    6;
			mp_nextspeed	=    6;
			mod_tempo		=  125;
			mp_nexttempo	=  125;
		}
		else
		{
			mp_done = 1;
			return;
		}
	}

	if(mp_globaltick == 0)
	{
		mp_patternset	= 0;
		mp_currow		= mp_row;
		mp_curpattern	= mp_pattern;

		mp_dmacopy(mod_patterns_data + ((uint16_t)(mod_patternlist[mp_curpattern]) << 10) + (mp_currow << 4), (uint32_t)mp_currowdata, 16);

		mp_preprocesseffects(&mp_currowdata[0 ]);
		mp_preprocesseffects(&mp_currowdata[4 ]);
		mp_preprocesseffects(&mp_currowdata[8 ]);
		mp_preprocesseffects(&mp_currowdata[12]);

		mod_speed = mp_nextspeed;
		mod_tempo = mp_nexttempo;
	}

	mp_processnote(0, &mp_currowdata[0 ]);
	mp_processnote(1, &mp_currowdata[4 ]);
	mp_processnote(2, &mp_currowdata[8 ]);
	mp_processnote(3, &mp_currowdata[12]);

	mp_globaltick++;
	if(mp_globaltick == mod_speed)
	{
		if(mp_delcount)
		{
			mp_inrepeat = 1;
			mp_delcount--;
		}
		else
		{
			mp_delset	= 0;
			mp_addflag	= 0;
			mp_inrepeat	= 0;

			if(mp_currow == mp_row)
				mp_row++;
		}

		mp_globaltick = 0;
	}
}

// ------------------------------------------------------------------------------------

void modplay_initmod(uint32_t attic_address, uint32_t sample_address)
{
	uint16_t i;
	uint8_t a;

	mod_sample_addr = sample_address;
	mod_attic_addr = attic_address;

	mp_dmacopy(mod_attic_addr + 1080, (uint32_t)mod_tmpbuf, 4);							// Check if 15 or 31 instrument mode (M.K.)

	mod_sigsize = 0;
	mod_numinstruments = 15;

	mod_tmpbuf[4] = 0;
	for(i = 0; i < MP_NUM_SIGS; i++)
	{
		for(a = 0; a < 4; a++)
			if(mod_tmpbuf[a] != mp_modsigs[i][a])
				break;
		if(a == 4)
		{
			mod_sigsize = 4;
			mod_numinstruments = 31;
		}
	}

	mod_patternlist_offset = 20 + mod_numinstruments * 30 + 2;
	mod_patterns_offset = mod_sigsize + mod_patternlist_offset + 128;					// 600 for 15 instruments, 1084 for 31

	mod_patterns_data = mod_attic_addr + mod_patterns_offset;
	mod_patternlist_data = mod_attic_addr + mod_patternlist_offset;

	for(i = 0; i < mod_numinstruments; i++)
	{
		// 2 bytes - sample length in words. multiply by 2 for byte length
		// 1 byte  - lower 4 bits for finetune, upper 4 not used, set to 0
		// 1 byte  - volume for sample ($00-$40)
		// 2 bytes - repeat point in words
		// 2 bytes - repeat length in words
		mp_dmacopy(mod_attic_addr + 20 + i * 30 + 22, (uint32_t)mod_tmpbuf, 8);			// Get instrument data
		sample_lengths      [i] = mod_tmpbuf[1] + (mod_tmpbuf[0] << 8);
		sample_lengths      [i] <<= 1;													// Redenominate instrument length into bytes

		// finetune is a signed-4bit-number. convert to signed-8-bit
		int8_t tempfinetune = mod_tmpbuf[2] & 0x0f;
		if(tempfinetune > 0x07) //0b00000111
			tempfinetune |= 0xf0; // 0b11110000

		sample_finetune     [i] = tempfinetune;
		sample_vol          [i] = mod_tmpbuf[3];										// Instrument volume
		sample_repeatpoint  [i] = mod_tmpbuf[5] + (mod_tmpbuf[4] << 8);					// Repeat start point and end point
		sample_repeatlength [i] = mod_tmpbuf[7] + (mod_tmpbuf[6] << 8);
	}

	mod_songlength = lpeek(mod_attic_addr + 20 + mod_numinstruments * 30 + 0);
	mp_song_loop_point = lpeek(mod_attic_addr + 20 + mod_numinstruments * 30 + 1);

	mod_numpatterns = 0;
	mp_dmacopy(mod_patternlist_data, (uint32_t)mod_patternlist, 128);
	for(i = 0; i < mod_songlength; i++)
	{
		if(mod_patternlist[i] > mod_numpatterns)
			mod_numpatterns = mod_patternlist[i];
	}

	// copy samples from attic ram to fast ram
	mod_sample_offset = mod_patterns_offset + ((uint32_t)mod_numpatterns + 1) * 1024;
	uint32_t mod_attic_sample_data = mod_attic_addr + mod_sample_offset;
	for(i = 0; i < 5; i++)
	{
		mp_dmacopy((mod_attic_sample_data + (uint32_t)i * 0x10000), (mod_sample_addr + (uint32_t)i * 0x10000), 0); // 65536
	}

	mod_sample_data = mod_sample_addr;
	for(i = 0; i < MP_MAX_INSTRUMENTS; i++)
	{
		sample_addr[i] = mod_sample_data;
		mod_sample_data += sample_lengths[i];
	}
	
	mp_row				= 0;
	mp_currow			= 0;
	mp_pattern			= 0;
	mp_curpattern		= 0;
	mp_delcount			= 0;
	mp_globaltick		= 0;
	mp_delset			= 0;
	mp_inrepeat			= 0;
	mp_addflag			= 0;
	mp_arpeggiocounter	= 0;
	mp_patternset		= 0;

	mod_speed			= 6;
	mp_nextspeed		= 6;
	mod_tempo			= 125;
	mp_nexttempo		= 125;

	mp_done				= 0;

	for(i = 0; i < 4; i++)
	{
		channel_volume				[i] = 0;
		channel_tempvolume			[i] = 0;
		channel_deltick				[i] = 0;
		channel_stop				[i] = 1;
		channel_repeat				[i] = 0;
		channel_period				[i] = 0;
		channel_portdest			[i] = 0;
		channel_tempperiod			[i] = 0;
		channel_portstep			[i] = 0;
		channel_offset				[i] = 0;
		channel_offsetmem			[i] = 0;
		channel_retrig				[i] = 0;
		channel_vibwave				[i] = 0;
		channel_tremwave			[i] = 0;
		channel_vibpos				[i] = 0;
		channel_trempos				[i] = 0;
		channel_looppoint			[i] = 0;
		channel_loopcount			[i] = -1;
		channel_sample				[i] = 0;
	}

	// finally, enable audio DMA again
	AUDIO_DMA.AUDEN		= 0b10000000;
}

// ------------------------------------------------------------------------------------

void modplay_init()
{
	uint16_t i;

	// turn off saturation
	AUDIO_DMA.DBGSAT	= 0b00000000;

	// set up dma values that don't change
	mp_dmacopyjob.command_lo		= 0x00; // copy
	mp_dmacopyjob.sourcemb_token	= 0x80;
	mp_dmacopyjob.destmb_token		= 0x81;
	mp_dmacopyjob.format			= 0x0b;
	mp_dmacopyjob.endtokenlist		= 0x00;
	mp_dmacopyjob.command_hi		= 0x00;
	mp_dmacopyjob.sourcemb			= 0x80; // modplay only does DMA copies from attic MB
	mp_dmacopyjob.destmb			= 0x00; // modplay only does DMA copies to fast MB

	// audioxbar_setcoefficient(i, 0xff);
	for(i = 0; i < 256; i++)
	{
		// Select the coefficient
		poke(0xd6f4, i);

		// Now wait at least 16 cycles for it to settle
		poke(0xd020, peek(0xd020));
		poke(0xd020, peek(0xd020));

		// set value to 0xff
		poke(0xd6f5, 0x3f);
	}

	modplay_disable();
	modplay_mute();
}

// ------------------------------------------------------------------------------------
