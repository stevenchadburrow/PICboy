
// PICboy.c

// To compile:
// gcc -o PICboy.o PICboy.c -lglfw -lGL -lopenal

// Will probably require some packages for OpenGL, GLFW, and OpenAL.

// PICboy
// A fast Gameboy (Color) Emulator designed for Microcontrollers
// Using OpenGL/GLFW for Video and Keyboard, and OpenAL for Audio
// Creator: Professor Steven Chad Burrow
// Email: stevenchadburrow@gmail.com
// GitHub: github.com/stevenchadburrow
// Public Domain, Mar 2026

// change to 0 or 1
#define AUDIO_ENABLE 1

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_X 160
#define SCREEN_Y 144
#define SCREEN_Z 2 // scale for OpenGL
#define AUDIO_LEN 2048 // at least 1098

// uses OpenGL for graphics and keyboard
#include <GLFW/glfw3.h>
#include <GL/gl.h>

GLFWwindow* opengl_window;
int opengl_size_x = SCREEN_X * SCREEN_Z;
int opengl_size_y = SCREEN_Y * SCREEN_Z;
int opengl_keyboard_state[512];

// uses OpenAL for audio
#include <AL/al.h>
#include <AL/alc.h>

ALCdevice *openal_device;
ALCcontext *openal_context;
ALuint openal_source;
ALuint openal_buffer;
unsigned short openal_data[AUDIO_LEN];
unsigned char openal_enable = 1;

// variables for general emulation
unsigned short gb_game_screen_buffer[SCREEN_X*SCREEN_Y];
unsigned short gb_game_audio_buffer[AUDIO_LEN];
int gb_game_audio_file = 0;
unsigned int gb_game_audio_read = 0;
unsigned int gb_game_audio_write = 0;
unsigned char gb_game_buttons_current = 0;
unsigned char gb_game_buttons_previous = 0;
unsigned char gb_game_buttons_turbo_a = 0;
unsigned char gb_game_buttons_turbo_b = 0;
unsigned char gb_game_buttons_turbo_timer = 0;
unsigned char gb_game_buttons_fast_forward = 0;
unsigned char gb_game_buttons_freeze_state = 0;
unsigned char gb_game_buttons_freeze_hold = 0;
unsigned char gb_game_buttons_saving = 0;
unsigned char gb_game_run = 1;
unsigned char gb_game_draw = 0;
unsigned long gb_game_clock = 0;

// variables for gameboy specific emulation

#define UNK 0
#define DMG 1
#define GBC 2

unsigned long gb_mode = UNK; // unknown to use DMG or GBC as default

// random number table for noise, etc
unsigned char gb_random_table[256] = {
	0xBF,0xA9,0x14,0xD5,0x98,0x52,0x4E,0x56,0xAB,0x80,0xEF,0x3C,0x3F,0x97,0x89,0x35,
	0x05,0x3A,0xD3,0xA0,0x68,0x25,0x21,0x00,0x92,0x8C,0x5C,0x73,0xB8,0x55,0x4A,0x6B,
	0xD2,0x23,0x65,0xE9,0xA8,0xFA,0x9A,0xF3,0xA3,0xFF,0xBA,0x04,0xEB,0x90,0x71,0x8F,
	0x58,0xBC,0xC9,0x44,0xED,0x5E,0x33,0xF5,0x2A,0x96,0x32,0x03,0x93,0x02,0x50,0xDD,
	0x6D,0x39,0x1E,0x99,0x51,0x82,0xEE,0x6F,0xD4,0xB0,0xF7,0x0D,0xDE,0x83,0x70,0xE7,
	0xC7,0x26,0x86,0x63,0xDB,0x37,0xFC,0xA4,0x1F,0x24,0x7D,0x4F,0x7C,0x0E,0xB5,0xE1,
	0x72,0x9D,0x77,0x6E,0xE5,0xA6,0x9C,0xC0,0xC1,0x1D,0xAC,0xB6,0x15,0xB7,0xA7,0x67,
	0x84,0xB3,0x3B,0x7E,0xE2,0x87,0x17,0x59,0x38,0x4D,0x0A,0x69,0x7A,0xC5,0x20,0x8E,
	0x22,0xA2,0x16,0x10,0xF2,0x3E,0x43,0xE6,0x53,0xD7,0xFB,0xC3,0xDC,0xD8,0x9F,0x49,
	0x28,0xBB,0xAE,0x07,0x0B,0x2D,0x1A,0xF0,0xB1,0xFD,0xD9,0x9E,0x29,0xDA,0xF9,0x27,
	0x7B,0xBD,0xB2,0x2F,0x88,0xB9,0x13,0x09,0xDF,0xEC,0xF8,0x75,0xF4,0xCE,0xF1,0x1B,
	0x41,0xD6,0x5B,0x5D,0xC6,0x42,0x76,0x4B,0x94,0x34,0x64,0x18,0xD1,0xE8,0x9B,0x78,
	0x6C,0x19,0xAA,0xC4,0x31,0x4C,0xCC,0xF6,0x54,0x06,0x5A,0xEA,0x12,0xD0,0x62,0x3D,
	0x8D,0xE3,0xCF,0x95,0xC2,0xCB,0xA1,0x01,0x2B,0x6A,0x5F,0x2C,0x11,0x85,0x74,0x45,
	0x30,0xCD,0xE0,0xFE,0x7F,0xB4,0x60,0xA5,0xE4,0x1C,0xAF,0x91,0x79,0x8A,0xAD,0x47,
	0x66,0x08,0x36,0xBE,0x40,0x46,0x48,0x2E,0x0F,0x8B,0x61,0x0C,0xCA,0x81,0xC8,0x57
};

unsigned short gb_palette_master[16] = {
	0x7FFF, 0x3D3F, 0x2108, 0x0000, // grey-scale
	0x7FFF, 0x7E10, 0x48E7, 0x0000, // red-scale
	0x7FFF, 0x3FE6, 0x0200, 0x0000, // green-scale
	0x7FFF, 0x329F, 0x001F, 0x0000 // blue-scale
};

unsigned short gb_palette_defined[12] = {
	0x7FFF, 0x3DEF, 0x2108, 0x0000, // bgp
	0x7FFF, 0x3DEF, 0x2108, 0x0000, // obp0
	0x7FFF, 0x3DEF, 0x2108, 0x0000 // obp1
};

// memory arrays
unsigned char gb_mem_rom[4194304]; // bigger?
unsigned char gb_mem_vram[16384];
unsigned char gb_mem_eram[32768]; // bigger?
unsigned char gb_mem_wram[32768];
unsigned char gb_mem_oam[160];
unsigned char gb_mem_wave[16];
unsigned char gb_mem_hram[127];

// memory banks
unsigned char gb_bank_vram = 0;
unsigned char gb_bank_wram = 1;

// cpu registers
union {
	struct {
		unsigned char f;
		unsigned char a;
	} r8;
	unsigned short r16;
} gb_reg_af;

union {
	struct {
		unsigned char c;
		unsigned char b;
	} r8;
	unsigned short r16;
} gb_reg_bc;

union {
	struct {
		unsigned char e;
		unsigned char d;
	} r8;
	unsigned short r16;
} gb_reg_de;

union {
	struct {
		unsigned char l;
		unsigned char h;
	} r8;
	unsigned short r16;
} gb_reg_hl;

union {
	struct {
		unsigned char low;
		unsigned char high;
	} r8;
	unsigned short r16;
} gb_reg_sp;

union {
	struct {
		unsigned char low;
		unsigned char high;
	} r8;
	unsigned short r16;
} gb_reg_pc;


// cpu values
unsigned char gb_cpu_opcode = 0;

union {
	struct {
		unsigned char low;
		unsigned char high;
	} r8;
	unsigned short r16;
} gb_cpu_operand;

union {
	struct {
		unsigned char low;
		unsigned char high;
	} r8;
	unsigned short r16;
} gb_cpu_pointer;

unsigned long gb_cpu_result = 0;
unsigned long gb_cpu_storage = 0;
unsigned long gb_cpu_bits = 0;
unsigned long gb_cpu_cycles = 0;
unsigned long gb_cpu_halt = 0;
unsigned long gb_cpu_ime = 0;

// i/o registers
unsigned long gb_io_joyp = 0;
unsigned long gb_io_sb = 0;
unsigned long gb_io_sc = 0;
unsigned long gb_io_div = 0;
unsigned long gb_io_tima = 0;
unsigned long gb_io_tma = 0;
unsigned long gb_io_tac = 0;
unsigned long gb_io_if = 0;
unsigned long gb_io_lcdc = 0;
unsigned long gb_io_stat = 0;
unsigned long gb_io_scy = 0;
unsigned long gb_io_scx = 0;
unsigned long gb_io_ly = 0;
unsigned long gb_io_lyc = 0;
unsigned long gb_io_dma = 0;
unsigned long gb_io_bgp = 0;
unsigned long gb_io_obp0 = 0;
unsigned long gb_io_obp1 = 0;
unsigned long gb_io_wy = 0;
unsigned long gb_io_wx = 0;
unsigned long gb_io_boot = 0;
unsigned long gb_io_ie = 0;

// audio registers
unsigned long gb_aud_nr10 = 0; // channel 1 sweep
unsigned long gb_aud_nr11 = 0; // channel 1 length/duty
unsigned long gb_aud_nr12 = 0; // channel 1 volume/envelope
unsigned long gb_aud_nr13 = 0; // channel 1 period low
unsigned long gb_aud_nr14 = 0; // channel 1 period high/control
unsigned long gb_aud_nr21 = 0; // channel 2 length/duty
unsigned long gb_aud_nr22 = 0; // channel 2 volume/envelope
unsigned long gb_aud_nr23 = 0; // channel 2 period low
unsigned long gb_aud_nr24 = 0; // channel 2 period high/control
unsigned long gb_aud_nr30 = 0; // channel 3 dac enable
unsigned long gb_aud_nr31 = 0; // channel 3 length
unsigned long gb_aud_nr32 = 0; // channel 3 volume
unsigned long gb_aud_nr33 = 0; // channel 3 period low
unsigned long gb_aud_nr34 = 0; // channel 4 period high/control
unsigned long gb_aud_nr41 = 0; // channel 4 length
unsigned long gb_aud_nr42 = 0; // channel 4 volume/envelope
unsigned long gb_aud_nr43 = 0; // channel 4 frequency
unsigned long gb_aud_nr44 = 0; // channel 4 control
unsigned long gb_aud_nr50 = 0; // master volume
unsigned long gb_aud_nr51 = 0; // sound panning
unsigned long gb_aud_nr52 = 0; // audio enable

// cart registers
unsigned long gb_cart_type = 0;
unsigned long gb_cart_mbc = 0;
unsigned long gb_cart_enable_ram = 0;
unsigned long gb_cart_mask_rom = 1;
unsigned long gb_cart_mask_ram = 0;
unsigned long gb_cart_bank_rom = 1;
unsigned long gb_cart_bank_ram = 0;
unsigned long gb_cart_bank_mode = 0;
unsigned long gb_cart_bank_addr = 0;

// extra registers
unsigned long gb_ext_lx = 0;
unsigned long gb_ext_halt = 0;
unsigned long gb_ext_div_cycles = 0;
unsigned long gb_ext_tima_cycles = 0;
unsigned long gb_ext_sb_cycles = 0;
unsigned long gb_ext_dma_addr = 0;
unsigned long gb_ext_aud_cycles = 0;
unsigned long gb_ext_ch1_sweep = 0;
unsigned long gb_ext_ch1_temp = 0;
unsigned long gb_ext_ch1_period = 0;
unsigned long gb_ext_ch1_length = 0;
unsigned long gb_ext_ch1_volume = 0;
unsigned long gb_ext_ch1_envelope = 0;
unsigned long gb_ext_ch1_value = 0;
unsigned long gb_ext_ch2_period = 0;
unsigned long gb_ext_ch2_length = 0;
unsigned long gb_ext_ch2_volume = 0;
unsigned long gb_ext_ch2_envelope = 0;
unsigned long gb_ext_ch2_value = 0;
unsigned long gb_ext_ch3_period = 0;
unsigned long gb_ext_ch3_length = 0;
unsigned long gb_ext_ch3_volume = 0;
unsigned long gb_ext_ch3_index = 0;
unsigned long gb_ext_ch3_value = 0;
unsigned long gb_ext_ch4_divider = 0;
unsigned long gb_ext_ch4_period = 0;
unsigned long gb_ext_ch4_length = 0;
unsigned long gb_ext_ch4_volume = 0;
unsigned long gb_ext_ch4_envelope = 0;
unsigned long gb_ext_ch4_value = 0;
unsigned long gb_ext_ch4_random = 0;

// waiting for proper timing
void gb_wait()
{
	while (clock() < gb_game_clock + 16742) { } // for 59.73 Hz
	gb_game_clock = clock();	
}

// cart RAM save function
int gb_save(const char *filename)
{
	FILE *f = NULL;

	f = fopen(filename, "wb");
	if (!f)
	{
		printf("Error writing to save file!\n");
		return 0;
	}

	// 32KB cart RAM, change later if need be
	for (int i=0; i<32768; i++)
	{
		fprintf(f, "%c", gb_mem_eram[i]);
	}

	fclose(f);

	return 1;
}

// cart RAM load function
int gb_load(const char *filename)
{
	FILE *f = NULL;

	f = fopen(filename, "rb");
	if (!f)
	{
		printf("Error reading from load file!\n");
		return 0;
	}

	// 32KB cart RAM, change later if need be
	for (int i=0; i<32768; i++)
	{
		fscanf(f, "%c", &gb_mem_eram[i]);
	}

	fclose(f);

	return 1;
}

// read, write, and step
#define gb_def_read_8(A, B) { \
	B = (unsigned char)gb_read((unsigned short)A); }

#define gb_def_write_8(A, B) { \
	gb_write((unsigned short)A, (unsigned char)B); }

#define gb_def_step(A, B) { \
	A = (unsigned short)((unsigned short)A + (signed short)B); }	

// 16-bit operations
#define gb_def_ld_16(A, B) { \
	gb_cpu_result = (unsigned long)(B & 0xFFFF); \
	A = (unsigned short)(gb_cpu_result); }

#define gb_def_inc_16(A) { \
	gb_cpu_result = (unsigned long)(((unsigned short)A + 1) & 0xFFFF); \
	A = (unsigned short)(gb_cpu_result); }

#define gb_def_dec_16(A) { \
	gb_cpu_result = (unsigned long)(((unsigned short)A - 1) & 0xFFFF); \
	A = (unsigned short)(gb_cpu_result); }

#define gb_def_add_16(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned short)A + (unsigned short)B); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x1000) ? 0x20 : 0x00); \
	A = (unsigned short)(gb_cpu_result); }

#define gb_def_pop_16(A) { \
	gb_def_read_8(gb_reg_sp.r16, gb_cpu_pointer.r8.low); \
	gb_def_step(gb_reg_sp.r16, 1); \
	gb_def_read_8(gb_reg_sp.r16, gb_cpu_pointer.r8.high); \
	gb_def_step(gb_reg_sp.r16, 1); \
	A = (unsigned short)gb_cpu_pointer.r16; }

#define gb_def_push_16(A) { \
	gb_cpu_pointer.r16 = (unsigned short)A; \
	gb_def_step(gb_reg_sp.r16, -1); \
	gb_def_write_8(gb_reg_sp.r16, gb_cpu_pointer.r8.high); \
	gb_def_step(gb_reg_sp.r16, -1); \
	gb_def_write_8(gb_reg_sp.r16, gb_cpu_pointer.r8.low); }

// 8-bit operations
#define gb_def_ld_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)B & 0xFF); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_inc_8(A) { \
	gb_cpu_result = (unsigned long)(((unsigned char)A + 1) & 0xFF); \
	A = (unsigned char)(gb_cpu_result); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= ((gb_cpu_result & 0x0F) == 0x00 ? 0x20 : 0x00); }

#define gb_def_dec_8(A) { \
	gb_cpu_result = (unsigned long)(((unsigned char)A - 1) & 0xFF); \
	A = (unsigned char)(gb_cpu_result); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= ((gb_cpu_result & 0x0F) == 0x0F ? 0x20 : 0x00); }

#define gb_def_rlc_8(A) { \
	A = (unsigned char)((A << 1) | (A >> 7)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= ((A & 0x01) ? 0x10 : 0x00); \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_sla_8(A) { \
	gb_cpu_storage = (unsigned char)((A & 0x80) >> 3); \
	A = (unsigned char)((A << 1)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= gb_cpu_storage; \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_rl_8(A) { \
	gb_cpu_storage = (unsigned char)((A & 0x80) >> 3); \
	A = (unsigned char)((A << 1) | ((gb_reg_af.r8.f & 0x10) >> 4)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= gb_cpu_storage; \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_rrc_8(A) { \
	A = (unsigned char)((A >> 1) | (A << 7)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= ((A & 0x80) ? 0x10 : 0x00); \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_sra_8(A) { \
	gb_cpu_storage = (unsigned char)((A & 0x01) << 4); \
	A = (unsigned char)((A >> 1) | (A & 0x80)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= gb_cpu_storage; \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_rr_8(A) { \
	gb_cpu_storage = (unsigned char)((A & 0x01) << 4); \
	A = (unsigned char)((A >> 1) | ((gb_reg_af.r8.f & 0x10) << 3)); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= gb_cpu_storage; \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_add_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A + (unsigned char)B); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x10) ? 0x20 : 0x00); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_adc_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A + (unsigned char)B + (unsigned char)((gb_reg_af.r8.f & 0x10) >> 4)); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x10) ? 0x20 : 0x00); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_sub_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A - (unsigned char)B); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x10) ? 0x20 : 0x00); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_sbc_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A - (unsigned char)B - (unsigned char)((gb_reg_af.r8.f & 0x10) >> 4)); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x10) ? 0x20 : 0x00); \
	A = (unsigned char)(gb_cpu_result);  }

#define gb_def_and_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A & (unsigned char)B); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_xor_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A ^ (unsigned char)B); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_or_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A | (unsigned char)B); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_cp_8(A, B) { \
	gb_cpu_result = (unsigned long)((unsigned char)A - (unsigned char)B); \
	gb_reg_af.r8.f &= 0xD0; \
	gb_reg_af.r8.f |= (((A ^ B ^ gb_cpu_result) & 0x10) ? 0x20 : 0x00); }

#define gb_def_swap_8(A) { \
	gb_cpu_storage = (unsigned long)(((unsigned char)A & 0xF0) >> 4); \
	gb_cpu_result = (unsigned long)(((unsigned char)A & 0x0F) << 4); \
	gb_cpu_result |= (unsigned long)(gb_cpu_storage); \
	A = (unsigned char)(gb_cpu_result); }

#define gb_def_srl_8(A) { \
	gb_cpu_storage = (unsigned char)((A & 0x01) << 4); \
	A = (unsigned char)((A >> 1) & 0x7F); \
	gb_reg_af.r8.f = 0x00; \
	gb_reg_af.r8.f |= gb_cpu_storage; \
	gb_reg_af.r8.f |= ((A & 0xFF) == 0x00 ? 0x80 : 0x00); }

#define gb_def_bit_8(A, B) { \
	gb_cpu_result = (unsigned long)(A & B); }

#define gb_def_res_8(A, B) { \
	A &= (unsigned char)(B); }

#define gb_def_set_8(A, B) { \
	A |= (unsigned char)(B); }

// flags
#define gb_def_flag_z_calc_16() { \
	gb_reg_af.r8.f &= 0x70; \
	gb_reg_af.r8.f |= ((gb_cpu_result & 0x0000FFFF) == 0x00000000 ? 0x80 : 0x00); }

#define gb_def_flag_z_calc_8() { \
	gb_reg_af.r8.f &= 0x70; \
	gb_reg_af.r8.f |= ((gb_cpu_result & 0x000000FF) == 0x00000000 ? 0x80 : 0x00); }

#define gb_def_flag_z_set() { \
	gb_reg_af.r8.f |= 0x80; }

#define gb_def_flag_z_clr() { \
	gb_reg_af.r8.f &= 0x70; }
	
#define gb_def_flag_n_set() { \
	gb_reg_af.r8.f |= 0x40; }

#define gb_def_flag_n_clr() { \
	gb_reg_af.r8.f &= 0xB0; }

#define gb_def_flag_h_set() { \
	gb_reg_af.r8.f |= 0x20; }

#define gb_def_flag_h_clr() { \
	gb_reg_af.r8.f &= 0xD0; }

#define gb_def_flag_c_calc_16() { \
	gb_reg_af.r8.f &= 0xE0; \
	gb_reg_af.r8.f |= (((unsigned long)gb_cpu_result & 0xFFFF0000) != 0x00000000 ? 0x10 : 0x00); }

#define gb_def_flag_c_calc_8() { \
	gb_reg_af.r8.f &= 0xE0; \
	gb_reg_af.r8.f |= (((unsigned long)gb_cpu_result & 0xFFFFFF00) != 0x00000000 ? 0x10 : 0x00); }

#define gb_def_flag_c_set() { \
	gb_reg_af.r8.f |= 0x10; }

#define gb_def_flag_c_clr() { \
	gb_reg_af.r8.f &= 0xE0; }

#define gb_def_cycles(A) { \
	gb_cpu_cycles += A; }

// initial values after boot
void gb_initialize()
{
	gb_cpu_halt = 0x00;
	gb_cpu_ime = 0x00;

	gb_reg_af.r8.a = 0x01;
	gb_reg_af.r8.f = 0x80;
	if (gb_mem_rom[0x014D] > 0) gb_reg_af.r8.f |= 0x60; 
	gb_reg_bc.r8.b = 0x00;
	gb_reg_bc.r8.c = 0x13;
	gb_reg_de.r8.d = 0x00;
	gb_reg_de.r8.e = 0xD8;
	gb_reg_hl.r8.h = 0x01;
	gb_reg_hl.r8.l = 0x4D;
	gb_reg_sp.r16 = 0xFFFE;
	gb_reg_pc.r16 = 0x0100;
	
	gb_io_joyp = 0xCF;

	gb_io_sb = 0x00;
	gb_io_sc = 0x7E;

	gb_io_div = 0xAB;
	gb_io_tima = 0x00;
	gb_io_tma = 0x00;
	gb_io_tac = 0xF8;

	gb_io_if = 0xE1;

	gb_aud_nr10 = 0x80;
	gb_aud_nr11 = 0xBF;
	gb_aud_nr12 = 0xF3;
	gb_aud_nr13 = 0xFF;
	gb_aud_nr14 = 0xBF;
	gb_aud_nr21 = 0x3F;
	gb_aud_nr22 = 0x00;
	gb_aud_nr23 = 0xFF;
	gb_aud_nr24 = 0xBF;
	gb_aud_nr30 = 0x7F;
	gb_aud_nr31 = 0xFF;
	gb_aud_nr32 = 0x9F;
	gb_aud_nr33 = 0xFF;
	gb_aud_nr34 = 0xBF;
	gb_aud_nr41 = 0xFF;
	gb_aud_nr42 = 0x00;
	gb_aud_nr43 = 0x00;
	gb_aud_nr44 = 0xBF;
	gb_aud_nr50 = 0x77;
	gb_aud_nr51 = 0xF3;
	gb_aud_nr52 = 0xF1;

	gb_io_lcdc = 0x91;
	gb_io_stat = 0x85;
	gb_io_scy = 0x00;
	gb_io_scx = 0x00;
	gb_io_ly = 0x00;
	gb_io_lyc = 0x00;
	gb_io_bgp = 0xFC;
	gb_io_obp0 = 0xFF;
	gb_io_obp1 = 0xFF;
	gb_io_wy = 0x00;
	gb_io_wx = 0x00;

	gb_io_boot = 0x01;

	gb_io_ie = 0x00;

	gb_cart_type = gb_mem_rom[0x0147];

	if (gb_cart_type == 0x00 || 
		gb_cart_type == 0x08 ||
		gb_cart_type == 0x09)
	{
		gb_cart_mbc = 0x00; // no mbc
	}
	else if (gb_cart_type == 0x01 ||
		gb_cart_type == 0x02 ||
		gb_cart_type == 0x03)
	{
		gb_cart_mbc = 0x01; // mbc1
	}
	else if (gb_cart_type == 0x05 ||
		gb_cart_type == 0x06)
	{
		gb_cart_mbc = 0x02; // mbc2
	}
	else if (gb_cart_type == 0x0F ||
		gb_cart_type == 0x10 ||
		gb_cart_type == 0x11 ||
		gb_cart_type == 0x12 ||
		gb_cart_type == 0x13)
	{
		gb_cart_mbc = 0x03; // mbc3
	}
	else if (gb_cart_type == 0x19 ||
		gb_cart_type == 0x1A ||
		gb_cart_type == 0x1B ||
		gb_cart_type == 0x1C ||
		gb_cart_type == 0x1D ||
		gb_cart_type == 0x1E)
	{
		gb_cart_mbc = 0x05; // mbc5
	}
	else
	{
		gb_cart_mbc = 0xFF; // other
	}

	if (gb_mem_rom[0x0148] <= 0x08)
	{
		gb_cart_mask_rom = (0x01 << (gb_mem_rom[0x0148]+2)) - 0x01;
	}
	else
	{
		gb_cart_mask_rom = 0x01;
	}

	gb_cart_mask_rom = (gb_cart_mask_rom << 13);
	gb_cart_mask_rom = (gb_cart_mask_rom | 0x00003FFF);
	
	switch (gb_mem_rom[0x0149])
	{
		case 0x00:
		{
			gb_cart_mask_ram = 0x00;
			break;
		}
		case 0x02:
		{
			gb_cart_mask_ram = 0x01;
			break;
		}
		case 0x03:
		{
			gb_cart_mask_ram = 0x03;
			break;
		}
		case 0x04:
		{
			gb_cart_mask_ram = 0x0F;
			break;
		}
		case 0x05:
		{
			gb_cart_mask_ram = 0x07;
			break;
		}
		default:
		{
			gb_cart_mask_ram = 0x00;
		}
	}

	gb_cart_mask_ram = (gb_cart_mask_ram << 12);
	gb_cart_mask_ram = (gb_cart_mask_ram | 0x00001FFF);

	if (gb_cart_mbc == 0x02)
	{
		gb_cart_mask_ram = 0x01FF; // 512 bytes of RAM
	}

	gb_cart_bank_rom = 1;
	gb_cart_bank_ram = 0;
	gb_cart_enable_ram = 0;

	// if unknown mode by default
	if (gb_mode == UNK)
	{
		if (gb_mem_rom[0x0143] == 0xC0) // GBC only
		{
			gb_mode = GBC;
		}
		else if (gb_mem_rom[0x0143] == 0x80) // GBC or DMG
		{
			gb_mode = GBC;
		}
		else // usually 0x00, DMG only
		{
			gb_mode = DMG;
		}
	}

	for (unsigned short i=0; i<8192; i++) gb_mem_vram[i] = 0x00;

	gb_ext_lx = 0;
	gb_ext_halt = 0;
	gb_ext_div_cycles = 0x00;
	gb_ext_tima_cycles = 0x00;
}

// read from memory
unsigned char gb_read(unsigned short addr)
{
	if (addr < 0x4000) // rom, fixed bank
	{
		switch (gb_cart_mbc)
		{
			case 0x00:
			{
				return gb_mem_rom[addr];
				break;
			}
			case 0x01:
			{
				if (gb_cart_bank_mode == 0x00)
				{
					return gb_mem_rom[addr];
				}
				else
				{
					gb_cart_bank_addr = (gb_cart_bank_ram << 19) | (addr & 0x3FFF);
					gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_rom);
					
					return gb_mem_rom[gb_cart_bank_addr];
				}

				break;	
			}
			case 0x02:
			{
				return gb_mem_rom[addr];
				break;
			}
			case 0x03:
			{
				return gb_mem_rom[addr];
				break;	
			}
			case 0x05:
			{
				return gb_mem_rom[addr];
				break;	
			}
			default:
			{
				return gb_mem_rom[addr];
				break;
			}
		}
	}
	else if (addr < 0x8000) // rom, switchable bank
	{
		switch (gb_cart_mbc)
		{
			case 0x00:
			{
				return gb_mem_rom[addr];
				break;
			}
			case 0x01:
			{
				gb_cart_bank_addr = (gb_cart_bank_ram << 19) | (gb_cart_bank_rom << 14) | (addr & 0x3FFF);
				gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_rom);

				return gb_mem_rom[gb_cart_bank_addr];		

				break;	
			}
			case 0x02:
			{
				gb_cart_bank_addr = (gb_cart_bank_rom << 14) | (addr & 0x3FFF);
				gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_rom);

				return gb_mem_rom[gb_cart_bank_addr];

				break;
			}
			case 0x03:
			{
				gb_cart_bank_addr = (gb_cart_bank_rom << 14) | (addr & 0x3FFF);
				gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_rom);

				return gb_mem_rom[gb_cart_bank_addr];		

				break;	
			}
			case 0x05:
			{
				gb_cart_bank_addr = (gb_cart_bank_rom << 14) | (addr & 0x3FFF);
				gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_rom);

				return gb_mem_rom[gb_cart_bank_addr];		

				break;	
			}
			default:
			{
				return gb_mem_rom[addr];
				break;
			}
		}
	}
	else if (addr < 0xA000) // video ram
	{
		return gb_mem_vram[(addr-0x8000)+0x2000*gb_bank_vram];
	}
	else if (addr < 0xC000) // external ram
	{
		switch (gb_cart_mbc)
		{
			case 0x00:
			{
				return gb_mem_eram[addr-0xA000];
				break;
			}
			case 0x01:
			{
				if (gb_cart_enable_ram > 0)
				{
					if (gb_cart_bank_mode == 0x00)
					{
						return gb_mem_eram[addr-0xA000];
					}
					else
					{
						gb_cart_bank_addr = (gb_cart_bank_ram << 13) | (addr & 0x1FFF);
						gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_ram);

						return gb_mem_eram[gb_cart_bank_addr];
					}
				}
				else
				{
					return 0xFF;
				}		

				break;	
			}
			case 0x02:
			{
				if (gb_cart_enable_ram > 0)
				{
					return gb_mem_eram[addr-0xA000];
				}
				else
				{
					return 0xFF;
				}		

				break;	
			}
			case 0x03:
			{
				if (gb_cart_enable_ram > 0)
				{
					if (gb_cart_bank_mode == 0x00)
					{
						return gb_mem_eram[addr-0xA000];
					}
					else
					{
						gb_cart_bank_addr = (gb_cart_bank_ram << 13) | (addr & 0x1FFF);
						gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_ram);

						return gb_mem_eram[gb_cart_bank_addr];
					}
				}
				else
				{
					return 0xFF;
				}		

				break;	
			}
			case 0x05:
			{
				if (gb_cart_enable_ram > 0)
				{
					if (gb_cart_bank_mode == 0x00)
					{
						return gb_mem_eram[addr-0xA000];
					}
					else
					{
						gb_cart_bank_addr = (gb_cart_bank_ram << 13) | (addr & 0x1FFF);
						gb_cart_bank_addr = (gb_cart_bank_addr & gb_cart_mask_ram);

						return gb_mem_eram[gb_cart_bank_addr];
					}
				}
				else
				{
					return 0xFF;
				}		

				break;	
			}
			default:
			{
				return gb_mem_eram[addr-0xA000];
				break;
			}
		}
	}
	else if (addr < 0xD000) // work ram, fixed bank
	{
		return gb_mem_wram[(addr-0xC000)];
	}
	else if (addr < 0xE000) // work ram, switchable bank
	{
		return gb_mem_wram[(addr-0xD000)+0x1000*gb_bank_wram];
	}
	else if (addr < 0xFE00) // echo ram
	{
		return gb_mem_wram[(addr-0xE000)+0x1000*gb_bank_wram];
	}
	else if (addr < 0xFEA0) // oam ram
	{
		return gb_mem_oam[(addr-0xFE00)];
	}
	else if (addr < 0xFF00) // not usable
	{
		return 0xFF;
	}
	else if (addr < 0xFF80) // i/o registers
	{
		// insert code here
		switch (addr)
		{
			case 0xFF00: // JOYP
			{
				if ((gb_io_joyp & 0x30) == 0x10)
				{
					return (0xC0 | (gb_io_joyp & 0x30) | (gb_game_buttons_current & 0x0F)); // buttons
				} 
				else if ((gb_io_joyp & 0x30) == 0x20)
				{
					return (0xC0 | (gb_io_joyp & 0x30) | ((gb_game_buttons_current & 0xF0) >> 4)); // d-pad
				}
				else
				{
					return (0xC0 | (gb_io_joyp & 0x30) | 0x0F);
				}
				break;
			}
			case 0xFF01: // SB
			{
				return 0xFF;
				break;
			}
			case 0xFF02: // SC
			{
				return (unsigned char)gb_io_sc;
				break;
			}
			case 0xFF04: // DIV
			{
				return (unsigned char)gb_io_div;
				break;
			}
			case 0xFF05: // TIMA
			{
				return (unsigned char)gb_io_tima;
				break;
			}
			case 0xFF06: // TMA
			{
				return (unsigned char)gb_io_tma;
				break;
			}
			case 0xFF07: // TAC
			{	
				return (unsigned char)gb_io_tac;
				break;
			}
			case 0xFF0F: // IF
			{
				return (unsigned char)gb_io_if;
				break;
			}
			case 0xFF10: // NR10
			{
				return (unsigned char)gb_aud_nr10;
				break;
			}
			case 0xFF11: // NR11
			{
				return (unsigned char)(gb_aud_nr11 & 0xC0);
				break;
			}
			case 0xFF12: // NR12
			{
				return (unsigned char)gb_aud_nr12;
				break;
			}
			case 0xFF13: // NR13
			{
				return 0x00;
				break;
			}
			case 0xFF14: // NR14
			{
				return (unsigned char)(gb_aud_nr14 & 0x40);
				break;
			}
			case 0xFF16: // NR21
			{
				return (unsigned char)(gb_aud_nr21 & 0xC0);
				break;
			}
			case 0xFF17: // NR22
			{
				return (unsigned char)gb_aud_nr22;
				break;
			}
			case 0xFF18: // NR23
			{
				return 0x00;
				break;
			}
			case 0xFF19: // NR24
			{
				return (unsigned char)(gb_aud_nr24 & 0x40);
				break;
			}
			case 0xFF1A: // NR30
			{
				return (unsigned char)gb_aud_nr30;
				break;
			}
			case 0xFF1B: // NR31
			{
				return 0x00;
				break;
			}
			case 0xFF1C: // NR32
			{
				return (unsigned char)gb_aud_nr32;
				break;
			}
			case 0xFF1D: // NR33
			{
				return 0x00;
				break;
			}
			case 0xFF1E: // NR34
			{
				return (unsigned char)(gb_aud_nr34 & 0x40);
				break;
			}
			case 0xFF20: // NR41
			{
				return 0x00;
				break;
			}
			case 0xFF21: // NR42
			{
				return (unsigned char)gb_aud_nr42;
				break;
			}
			case 0xFF22: // NR43
			{
				return (unsigned char)gb_aud_nr43;
				break;
			}
			case 0xFF23: // NR44
			{
				return (unsigned char)(gb_aud_nr44 & 0x40);
				break;
			}
			case 0xFF24: // NR50
			{
				return (unsigned char)gb_aud_nr50;
				break;
			}
			case 0xFF25: // NR51
			{
				return (unsigned char)gb_aud_nr51;
				break;
			}
			case 0xFF26: // NR52
			{
				return (unsigned char)gb_aud_nr52;
				break;
			}
			case 0xFF30: { return (unsigned char)gb_mem_wave[0]; break; } // WAVE
			case 0xFF31: { return (unsigned char)gb_mem_wave[1]; break; }
			case 0xFF32: { return (unsigned char)gb_mem_wave[2]; break; }
			case 0xFF33: { return (unsigned char)gb_mem_wave[3]; break; }
			case 0xFF34: { return (unsigned char)gb_mem_wave[4]; break; }
			case 0xFF35: { return (unsigned char)gb_mem_wave[5]; break; }
			case 0xFF36: { return (unsigned char)gb_mem_wave[6]; break; }
			case 0xFF37: { return (unsigned char)gb_mem_wave[7]; break; }
			case 0xFF38: { return (unsigned char)gb_mem_wave[8]; break; }
			case 0xFF39: { return (unsigned char)gb_mem_wave[9]; break; }
			case 0xFF3A: { return (unsigned char)gb_mem_wave[10]; break; }
			case 0xFF3B: { return (unsigned char)gb_mem_wave[11]; break; }
			case 0xFF3C: { return (unsigned char)gb_mem_wave[12]; break; }
			case 0xFF3D: { return (unsigned char)gb_mem_wave[13]; break; }
			case 0xFF3E: { return (unsigned char)gb_mem_wave[14]; break; }
			case 0xFF3F: { return (unsigned char)gb_mem_wave[15]; break; }
			case 0xFF40: // LCDC
			{
				return (unsigned char)gb_io_lcdc;
				break;
			}
			case 0xFF41: // STAT
			{
				return (unsigned char)gb_io_stat;
				break;
			}
			case 0xFF42: // SCY
			{
				return (unsigned char)gb_io_scy;
				break;
			}
			case 0xFF43: // SCX
			{
				return (unsigned char)gb_io_scx;
				break;
			}
			case 0xFF44: // LY
			{
				return (unsigned char)gb_io_ly;
				break;
			}
			case 0xFF45: // LYC
			{
				return (unsigned char)gb_io_lyc;
				break;
			}
			case 0xFF46: // DMA
			{
				return (unsigned char)gb_io_dma;
				break;
			}
			case 0xFF47: // BGP
			{
				return (unsigned char)gb_io_bgp;
				break;
			}
			case 0xFF48: // OBP0
			{
				return (unsigned char)gb_io_obp0;
				break;
			}
			case 0xFF49: // OBP1
			{
				return (unsigned char)gb_io_obp1;
				break;
			}
			case 0xFF4A: // WY
			{
				return (unsigned char)gb_io_wy;
				break;
			}
			case 0xFF4B: // WX
			{
				return (unsigned char)gb_io_wx;
				break;
			}
			case 0xFF50: // BOOT
			{
				return (unsigned char)gb_io_boot;
				break;
			}
			default:
			{
			}
		}
	}
	else if (addr < 0xFFFF) // hram
	{
		return gb_mem_hram[(addr-0xFF80)];
	}
	else // interrupt enable register
	{
		return (unsigned char)gb_io_ie;
	}

	return 0x00; // default
}

// write to memory
void gb_write(unsigned short addr, unsigned char val)
{
	if (addr < 0x4000) // rom, fixed bank
	{
		switch (gb_cart_mbc)
		{
			case 0x00:
			{
				break;
			}
			case 0x01:
			{
				if (addr < 0x2000)
				{
					if ((val & 0x0F) == 0x0A)
					{
						gb_cart_enable_ram = 1;
					}
					else
					{
						gb_cart_enable_ram = 0;
					}
				}
				else
				{
					gb_cart_bank_rom = (unsigned char)(val & 0x1F);
		
					if ((gb_cart_bank_rom & gb_cart_mask_rom) == 0x00)
					{
						gb_cart_bank_rom = 0x01;
					}
				}

				break;
			}
			case 0x02:
			{
				if ((addr & 0x0100) == 0x0000)
				{
					if ((val & 0x0F) == 0x0A)
					{
						gb_cart_enable_ram = 1;
					}
					else
					{
						gb_cart_enable_ram = 0;
					}
				}
				else
				{
					gb_cart_bank_rom = (unsigned char)(val & 0x0F);
		
					if ((gb_cart_bank_rom & gb_cart_mask_rom) == 0x00)
					{
						gb_cart_bank_rom = 0x01;
					}
				}

				break;
			}
			case 0x03:
			{
				if (addr < 0x2000)
				{
					if ((val & 0x0F) == 0x0A)
					{
						gb_cart_enable_ram = 1;
					}
					else
					{
						gb_cart_enable_ram = 0;
					}
				}
				else
				{
					gb_cart_bank_rom = (unsigned char)(val & 0x7F);
		
					if (gb_cart_bank_rom == 0x00)
					{
						gb_cart_bank_rom = 0x01;
					}
				}
	
				break;
			}
			case 0x05:
			{
				if (addr < 0x2000)
				{
					if ((val & 0x0F) == 0x0A)
					{
						gb_cart_enable_ram = 1;
					}
					else
					{
						gb_cart_enable_ram = 0;
					}
				}
				else
				{
					gb_cart_bank_rom = (unsigned short)((gb_cart_bank_rom & 0x0100) | (val & 0x00FF));
				}
	
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else if (addr < 0x8000) // rom, switchable bank
	{
		switch (gb_cart_mbc)
		{
			case 0x00:
			{
				break;
			}
			case 0x01:
			{
				if (addr < 0x6000)
				{
					gb_cart_bank_ram = (unsigned char)(val & 0x03);
				}
				else
				{
					gb_cart_bank_mode = (unsigned char)(val & 0x01);
				}

				break;
			}
			case 0x02:
			{
				break;
			}
			case 0x03:
			{
				if (addr < 0x6000)
				{
					gb_cart_bank_ram = (unsigned char)(val & 0x07);
				}
				else
				{
					// RTC operations
				}

				break;
			}
			case 0x05:
			{
				if (addr < 0x6000)
				{
					gb_cart_bank_rom = (unsigned short)((gb_cart_bank_rom & 0x00FF) | ((val & 0x0001) << 8));
				}
				else
				{
					gb_cart_bank_ram = (unsigned char)(val & 0x0F);
				}

				break;
			}
			default:
			{
				break;
			}
		}
	}
	else if (addr < 0xA000) // video ram
	{
		gb_mem_vram[(addr-0x8000)+0x2000*gb_bank_vram] = val;
	}
	else if (addr < 0xC000) // external ram
	{
		gb_mem_eram[addr-0xA000] = val;
	}
	else if (addr < 0xD000) // work ram, fixed bank
	{
		gb_mem_wram[(addr-0xC000)] = val;
	}
	else if (addr < 0xE000) // work ram, switchable bank
	{
		gb_mem_wram[(addr-0xD000)+0x1000*gb_bank_wram] = val;
	}
	else if (addr < 0xFE00) // echo ram, do nothing
	{
		gb_mem_wram[(addr-0xE000)+0x1000*gb_bank_wram] = val;
	}
	else if (addr < 0xFEA0) // oam ram
	{
		gb_mem_oam[(addr-0xFE00)] = val;
	}
	else if (addr < 0xFF00) // not usable
	{
	}
	else if (addr < 0xFF80) // i/o registers
	{
		switch (addr)
		{
			case 0xFF00: // JOYP
			{
				gb_io_joyp = (unsigned char)(val & 0x30);
				break;
			}
			case 0xFF01: // SB
			{	
				gb_io_sb = (unsigned char)val;
				break;
			}
			case 0xFF02: // SC
			{
				gb_io_sc = (unsigned char)val;
				break;
			}
			case 0xFF04: // DIV
			{
				gb_io_div = 0x00;
				gb_ext_div_cycles = 0x00;
				break;
			}
			case 0xFF05: // TIMA
			{
				gb_io_tima = (unsigned char)val;
				break;
			}
			case 0xFF06: // TMA
			{
				gb_io_tma = (unsigned char)val;
				break;
			}
			case 0xFF07: // TAC
			{
				gb_io_tac = (unsigned char)val;
				break;
			}
			case 0xFF0F: // IF
			{
				gb_io_if = (unsigned char)val;
				break;
			}
			case 0xFF10: // NR10
			{
				gb_aud_nr10 = (unsigned char)val;

				break;
			}
			case 0xFF11: // NR11
			{
				gb_aud_nr11 = (unsigned char)val;

				gb_ext_ch1_length = (unsigned long)((gb_aud_nr11 & 0x3F) << 7);

				break;
			}
			case 0xFF12: // NR12
			{
				gb_aud_nr12 = (unsigned char)val;

				gb_ext_ch1_volume = (unsigned long)((gb_aud_nr12 & 0xF0) >> 4);

				break;
			}
			case 0xFF13: // NR13
			{
				gb_aud_nr13 = (unsigned char)val;

				gb_ext_ch1_period = (unsigned long)(((gb_aud_nr14 & 0x07) << 8) | (gb_aud_nr13 & 0xFF));

				break;
			}
			case 0xFF14: // NR14
			{
				gb_aud_nr14 = (unsigned char)val;

				if ((gb_aud_nr14 & 0x80) == 0x80)
				{
					gb_aud_nr52 = (unsigned char)(gb_aud_nr52 | 0x01); // enable channel 1

					gb_ext_ch1_period = (unsigned long)(((gb_aud_nr14 & 0x07) << 8) | (gb_aud_nr13 & 0xFF));

					gb_ext_ch1_length = (unsigned long)((gb_aud_nr11 & 0x3F) << 7);

					gb_ext_ch1_volume = (unsigned long)((gb_aud_nr12 & 0xF0) >> 4);

					gb_ext_ch1_envelope = (unsigned long)(gb_aud_nr12 & 0x07);
				}

				break;
			}
			case 0xFF16: // NR21
			{
				gb_aud_nr21 = (unsigned char)val;

				gb_ext_ch2_length = (unsigned long)((gb_aud_nr21 & 0x3F) << 7);

				break;
			}
			case 0xFF17: // NR22
			{
				gb_aud_nr22 = (unsigned char)val;

				gb_ext_ch2_volume = (unsigned long)((gb_aud_nr22 & 0xF0) >> 4);

				gb_ext_ch2_envelope = (unsigned long)(gb_aud_nr22 & 0x07);

				break;
			}
			case 0xFF18: // NR23
			{
				gb_aud_nr23 = (unsigned char)val;

				gb_ext_ch2_period = (unsigned long)(((gb_aud_nr24 & 0x07) << 8) | (gb_aud_nr23 & 0xFF));

				break;
			}
			case 0xFF19: // NR24
			{
				gb_aud_nr24 = (unsigned char)val;

				if ((gb_aud_nr24 & 0x80) == 0x80)
				{
					gb_aud_nr52 = (unsigned char)(gb_aud_nr52 | 0x02); // enable channel 2

					gb_ext_ch2_period = (unsigned long)(((gb_aud_nr24 & 0x07) << 8) | (gb_aud_nr23 & 0xFF));

					gb_ext_ch2_length = (unsigned long)((gb_aud_nr21 & 0x3F) << 7);

					gb_ext_ch2_volume = (unsigned long)((gb_aud_nr22 & 0xF0) >> 4);

					gb_ext_ch2_envelope = (unsigned long)(gb_aud_nr22 & 0x07);
				}

				break;
			}
			case 0xFF1A: // NR30
			{
				gb_aud_nr30 = (unsigned char)val;

				break;
			}
			case 0xFF1B: // NR31
			{
				gb_aud_nr31 = (unsigned char)val;

				gb_ext_ch3_length = (unsigned long)(gb_aud_nr31 << 7);

				break;
			}
			case 0xFF1C: // NR32
			{
				gb_aud_nr32 = (unsigned char)val;

				switch ((gb_aud_nr32 & 0x60))
				{
					case 0x00:
					{
						gb_ext_ch3_volume = 0x00;
						break;
					}
					case 0x20:
					{
						gb_ext_ch3_volume = 0x0F;	
						break;
					}
					case 0x40:
					{
						gb_ext_ch3_volume = 0x07;
						break;
					}
					case 0x60:
					{
						gb_ext_ch3_volume = 0x03;
						break;
					}			
				}

				break;
			}
			case 0xFF1D: // NR33
			{
				gb_aud_nr33 = (unsigned char)val;

				gb_ext_ch3_period = (unsigned long)(((gb_aud_nr34 & 0x07) << 8) | (gb_aud_nr33 & 0xFF));

				break;
			}
			case 0xFF1E: // NR34
			{
				gb_aud_nr34 = (unsigned char)val;

				if ((gb_aud_nr34 & 0x80) == 0x80)
				{
					gb_aud_nr52 = (unsigned char)(gb_aud_nr52 | 0x04); // enable channel 3

					gb_ext_ch3_period = (unsigned long)(((gb_aud_nr34 & 0x07) << 8) | (gb_aud_nr33 & 0xFF));

					gb_ext_ch3_length = (unsigned long)(gb_aud_nr31 << 7);

					switch ((gb_aud_nr32 & 0x60))
					{
						case 0x00:
						{
							gb_ext_ch3_volume = 0x00;
							break;
						}
						case 0x20:
						{
							gb_ext_ch3_volume = 0x0F;	
							break;
						}
						case 0x40:
						{
							gb_ext_ch3_volume = 0x07;
							break;
						}
						case 0x60:
						{
							gb_ext_ch3_volume = 0x03;
							break;
						}			
					}

					gb_ext_ch3_index = 0;
				}

				break;
			}
			case 0xFF20: // NR41
			{
				gb_aud_nr41 = (unsigned char)val;

				gb_ext_ch4_length = (unsigned long)((gb_aud_nr41 & 0x3F) << 7);

				break;
			}
			case 0xFF21: // NR42
			{
				gb_aud_nr42 = (unsigned char)val;

				gb_ext_ch4_volume = (unsigned long)((gb_aud_nr42 & 0xF0) >> 4);

				gb_ext_ch4_envelope = (unsigned long)(gb_aud_nr42 & 0x07);

				break;
			}
			case 0xFF22: // NR43
			{
				gb_aud_nr43 = (unsigned char)val;

				gb_ext_ch4_period = 0x0000;

				gb_ext_ch4_divider = (unsigned long)((gb_aud_nr43 & 0x07) << ((gb_aud_nr43 & 0xF0) >> 8));

				break;
			}
			case 0xFF23: // NR44
			{
				gb_aud_nr44 = (unsigned char)val;

				if ((gb_aud_nr44 & 0x80) == 0x80)
				{
					gb_aud_nr52 = (unsigned char)(gb_aud_nr52 | 0x08); // enable channel 4

					gb_ext_ch4_period = 0x0000;

					gb_ext_ch4_length = (unsigned long)((gb_aud_nr41 & 0x3F) << 7);

					gb_ext_ch4_volume = (unsigned long)((gb_aud_nr42 & 0xF0) >> 4);

					gb_ext_ch4_envelope = (unsigned long)(gb_aud_nr42 & 0x07);
				}

				break;
			}
			case 0xFF24: // NR50
			{
				gb_aud_nr50 = (unsigned char)val;
				break;
			}
			case 0xFF25: // NR51
			{
				gb_aud_nr51 = (unsigned char)val;
				break;
			}
			case 0xFF26: // NR52
			{
				gb_aud_nr52 = (unsigned char)((gb_aud_nr52 & 0x7F) | (val & 0x80));

				if ((gb_aud_nr52 & 0x80) == 0x00)
				{
					gb_aud_nr10 = 0x00;
					gb_aud_nr11 = 0x00;
					gb_aud_nr12 = 0x00;
					gb_aud_nr13 = 0x00;
					gb_aud_nr14 = 0x00;
					gb_aud_nr21 = 0x00;
					gb_aud_nr22 = 0x00;
					gb_aud_nr23 = 0x00;
					gb_aud_nr24 = 0x00;
					gb_aud_nr30 = 0x00;
					gb_aud_nr31 = 0x00;
					gb_aud_nr32 = 0x00;
					gb_aud_nr33 = 0x00;
					gb_aud_nr34 = 0x00;
					gb_aud_nr41 = 0x00;
					gb_aud_nr42 = 0x00;
					gb_aud_nr43 = 0x00;
					gb_aud_nr44 = 0x00;
					gb_aud_nr50 = 0x00;
					gb_aud_nr51 = 0x00;
				}

				break;
			}
			case 0xFF30: { gb_mem_wave[0] = (unsigned char)val; break; } // WAVE
			case 0xFF31: { gb_mem_wave[1] = (unsigned char)val; break; }
			case 0xFF32: { gb_mem_wave[2] = (unsigned char)val; break; }
			case 0xFF33: { gb_mem_wave[3] = (unsigned char)val; break; }
			case 0xFF34: { gb_mem_wave[4] = (unsigned char)val; break; }
			case 0xFF35: { gb_mem_wave[5] = (unsigned char)val; break; }
			case 0xFF36: { gb_mem_wave[6] = (unsigned char)val; break; }
			case 0xFF37: { gb_mem_wave[7] = (unsigned char)val; break; }
			case 0xFF38: { gb_mem_wave[8] = (unsigned char)val; break; }
			case 0xFF39: { gb_mem_wave[9] = (unsigned char)val; break; }
			case 0xFF3A: { gb_mem_wave[10] = (unsigned char)val; break; }
			case 0xFF3B: { gb_mem_wave[11] = (unsigned char)val; break; }
			case 0xFF3C: { gb_mem_wave[12] = (unsigned char)val; break; }
			case 0xFF3D: { gb_mem_wave[13] = (unsigned char)val; break; }
			case 0xFF3E: { gb_mem_wave[14] = (unsigned char)val; break; }
			case 0xFF3F: { gb_mem_wave[15] = (unsigned char)val; break; }
			case 0xFF40: // LCDC
			{
				gb_io_lcdc = (unsigned char)val;
				break;
			}
			case 0xFF41: // STAT
			{
				gb_io_stat = (unsigned char)((gb_io_stat & 0x07) | (val & 0xF8));
				break;
			}
			case 0xFF42: // SCY
			{
				gb_io_scy = (unsigned char)val;
				break;
			}
			case 0xFF43: // SCX
			{
				gb_io_scx = (unsigned char)val;
				break;
			}
			case 0xFF44: // LY
			{
				break;
			}
			case 0xFF45: // LYC
			{
				gb_io_lyc = (unsigned char)val;
				break;
			}
			case 0xFF46: // DMA
			{
				gb_io_dma = (unsigned char)val;
				gb_ext_dma_addr = (val << 8);
				for (unsigned char i=0x00; i<0xA0; i++)
				{
					gb_write(0xFE00+i, gb_read(gb_ext_dma_addr+i));
				}
				break;
			}
			case 0xFF47: // BGP
			{
				gb_io_bgp = (unsigned char)val;
				break;
			}
			case 0xFF48: // OBP0
			{
				gb_io_obp0 = (unsigned char)val;
				break;
			}
			case 0xFF49: // OBP1
			{
				gb_io_obp1 = (unsigned char)val;
				break;
			}
			case 0xFF4A: // WY
			{
				gb_io_wy = (unsigned char)val;
				break;
			}
			case 0xFF4B: // WX
			{
				gb_io_wx = (unsigned char)val;
				break;
			}
			case 0xFF50: // BOOT
			{
				gb_io_boot = (unsigned char)val;
				break;
			}
			default:
			{
			}
		}
	}
	else if (addr < 0xFFFF) // hram
	{
		gb_mem_hram[(addr-0xFF80)] = val;
	}
	else // interrupt enable register
	{
		gb_io_ie = (unsigned char)val;
	}
}

// runs next instruction
void gb_run()
{
	// if halted, do not run cpu
	if (gb_cpu_halt > 0)
	{
		gb_def_cycles(4);
		return;
	}	

	gb_def_read_8(gb_reg_pc.r16, gb_cpu_opcode);

	gb_def_step(gb_reg_pc.r16, 1);

	// Unprefixed Codes

	switch (gb_cpu_opcode)
	{
		//0x00:{opcode:NOP,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x00:
		{
			// NOP
			gb_def_cycles(4);
			break;
		}
		//0x01:{opcode:LD,bytes:3,cycles:[12],operands:[{name:BC,imm:true},{name:n16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x01:
		{
			// LD BC,NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_16(gb_reg_bc.r16, gb_cpu_operand.r16);
			gb_def_cycles(12);
			break;
		}
		//0x02:{opcode:LD,bytes:1,cycles:[8],operands:[{name:BC,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x02:
		{
			// LD [BC],A
			gb_def_write_8(gb_reg_bc.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x03:{opcode:INC,bytes:1,cycles:[8],operands:[{name:BC,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x03:
		{
			// INC BC
			gb_def_inc_16(gb_reg_bc.r16);
			gb_def_cycles(8);
			break;
		}
		//0x04:{opcode:INC,bytes:1,cycles:[4],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x04:
		{
			// INC B
			gb_def_inc_8(gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x05:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x05:
		{
			// DEC B
			gb_def_dec_8(gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x06:{opcode:LD,bytes:2,cycles:[8],operands:[{name:B,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x06:
		{
			// LD B,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_bc.r8.b, gb_cpu_operand.r8.high);
			gb_def_cycles(8);
			break;
		}
		//0x07:{opcode:RLCA,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:0,N:0,H:0,C:C}}
		case 0x07:
		{
			// RLCA
			gb_reg_af.r8.a = (gb_reg_af.r8.a << 1) | (gb_reg_af.r8.a >> 7);
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f = ((gb_reg_af.r8.a & 0x01) << 4);
			gb_def_cycles(4);
			break;
		}
		//0x08:{opcode:LD,bytes:3,cycles:[20],operands:[{name:a16,bytes:2,imm:false},{name:SP,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x08:
		{
			// LD [NN],SP
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_write_8(gb_cpu_operand.r16, gb_reg_sp.r8.low);
			gb_def_step(gb_cpu_operand.r16, 1);
			gb_def_write_8(gb_cpu_operand.r16, gb_reg_sp.r8.high);
			gb_def_cycles(20);
			break;
		}
		//0x09:{opcode:ADD,bytes:1,cycles:[8],operands:[{name:HL,imm:true},{name:BC,imm:true}],imm:true,flags:{Z:-,N:0,H:H,C:C}}
		case 0x09:
		{
			// ADD HL,BC
			gb_def_add_16(gb_reg_hl.r16, gb_reg_bc.r16);
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_16();
			gb_def_cycles(8);
			break;
		}
		//0x0A:{opcode:LD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:BC,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x0A:
		{
			// LD A,[BC]
			gb_def_read_8(gb_reg_bc.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x0B:{opcode:DEC,bytes:1,cycles:[8],operands:[{name:BC,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x0B:
		{
			// DEC BC
			gb_def_dec_16(gb_reg_bc.r16);
			gb_def_cycles(8);
			break;
		}	
		//0x0C:{opcode:INC,bytes:1,cycles:[4],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x0C:
		{
			// INC C
			gb_def_inc_8(gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x0D:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x0D:
		{
			// DEC C
			gb_def_dec_8(gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x0E:{opcode:LD,bytes:2,cycles:[8],operands:[{name:C,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x0E:
		{
			// LD C,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_bc.r8.c, gb_cpu_operand.r8.low);
			gb_def_cycles(8);
			break;
		}
		//0x0F:{opcode:RRCA,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:0,N:0,H:0,C:C}}
		case 0x0F:
		{
			// RRCA
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f |= ((gb_reg_af.r8.a & 0x01) == 0x01 ? 0x10 : 0x00);
			gb_reg_af.r8.a = (gb_reg_af.r8.a >> 1) | (gb_reg_af.r8.a << 7);
			gb_def_cycles(4);
			break;
		}
		//0x10:{opcode:STOP,bytes:2,cycles:[4],operands:[{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x10:
		{
			//printf("STOP\n");

			// STOP
			gb_io_div = 0x00;
			gb_ext_div_cycles = 0x00;
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_cycles(4);
			break;
		}	
		//0x11:{opcode:LD,bytes:3,cycles:[12],operands:[{name:DE,imm:true},{name:n16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x11:
		{
			// LD DE,NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_16(gb_reg_de.r16, gb_cpu_operand.r16);
			gb_def_cycles(12);
			break;
		}
		//0x12:{opcode:LD,bytes:1,cycles:[8],operands:[{name:DE,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x12:
		{
			// LD [DE],A
			gb_def_write_8(gb_reg_de.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x13:{opcode:INC,bytes:1,cycles:[8],operands:[{name:DE,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x13:
		{
			// INC DE
			gb_def_inc_16(gb_reg_de.r16);
			gb_def_cycles(8);
			break;
		}
		//0x14:{opcode:INC,bytes:1,cycles:[4],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x14:
		{
			// INC D
			gb_def_inc_8(gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x15:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x15:
		{
			// DEC D
			gb_def_dec_8(gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x16:{opcode:LD,bytes:2,cycles:[8],operands:[{name:D,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x16:
		{
			// LD D,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_de.r8.d, gb_cpu_operand.r8.high);
			gb_def_cycles(8);
			break;
		}
		//0x17:{opcode:RLA,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:0,N:0,H:0,C:C}}
		case 0x17:
		{
			// RLA
			gb_cpu_operand.r8.low = ((gb_reg_af.r8.a & 0x80) >> 3);
			gb_reg_af.r8.a = (gb_reg_af.r8.a << 1) | ((gb_reg_af.r8.f & 0x10) >> 4);
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f |= gb_cpu_operand.r8.low;
			gb_def_cycles(4);
			break;
		}
		//0x18:{opcode:JR,bytes:2,cycles:[12],operands:[{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x18:
		{
			// JR N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_reg_pc.r16 = (unsigned short)((unsigned short)gb_reg_pc.r16 + (signed char)gb_cpu_operand.r8.low);
			gb_def_cycles(12);
			break;
		}
		//0x19:{opcode:ADD,bytes:1,cycles:[8],operands:[{name:HL,imm:true},{name:DE,imm:true}],imm:true,flags:{Z:-,N:0,H:H,C:C}}
		case 0x19:
		{
			// ADD HL,DE
			gb_def_add_16(gb_reg_hl.r16, gb_reg_de.r16);
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_16();
			gb_def_cycles(8);
			break;
		}
		//0x1A:{opcode:LD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:DE,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x1A:
		{
			// LD A,[DE]
			gb_def_read_8(gb_reg_de.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x1B:{opcode:DEC,bytes:1,cycles:[8],operands:[{name:DE,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x1B:
		{
			// DEC DE
			gb_def_dec_16(gb_reg_de.r16);
			gb_def_cycles(8);
			break;
		}
		//0x1C:{opcode:INC,bytes:1,cycles:[4],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x1C:
		{
			// INC E
			gb_def_inc_8(gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x1D:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x1D:
		{
			// DEC E
			gb_def_dec_8(gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x1E:{opcode:LD,bytes:2,cycles:[8],operands:[{name:E,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x1E:
		{
			// LD E,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_de.r8.e, gb_cpu_operand.r8.low);
			gb_def_cycles(8);
			break;
		}
		//0x1F:{opcode:RRA,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:0,N:0,H:0,C:C}}
		case 0x1F:
		{
			// RRA
			gb_cpu_operand.r8.low = ((gb_reg_af.r8.a & 0x01) << 4);
			gb_reg_af.r8.a = (gb_reg_af.r8.a >> 1) | ((gb_reg_af.r8.f & 0x10) << 3);
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f |= gb_cpu_operand.r8.low;
			gb_def_cycles(4);
			break;
		}
		//0x20:{opcode:JR,bytes:2,cycles:[12,8],operands:[{name:NZ,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x20:
		{
			// JRNZ N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x00)
			{
				gb_reg_pc.r16 = (unsigned short)((unsigned short)gb_reg_pc.r16 + (signed char)gb_cpu_operand.r8.low);
				gb_def_cycles(12);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}	
		//0x21:{opcode:LD,bytes:3,cycles:[12],operands:[{name:HL,imm:true},{name:n16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x21:
		{
			// LD HL,NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_16(gb_reg_hl.r16, gb_cpu_operand.r16);
			gb_def_cycles(12);
			break;
		}
		//0x22:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,inc:true,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x22:
		{
			// LD [HL+],A
			gb_def_write_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_reg_hl.r16 = (unsigned short)((unsigned short)gb_reg_hl.r16 + 1);
			gb_def_cycles(8);
			break;
		}
		//0x23:{opcode:INC,bytes:1,cycles:[8],operands:[{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x23:
		{
			// INC HL
			gb_def_inc_16(gb_reg_hl.r16);
			gb_def_cycles(8);
			break;
		}
		//0x24:{opcode:INC,bytes:1,cycles:[4],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x24:
		{
			// INC H
			gb_def_inc_8(gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x25:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x25:
		{
			// DEC H
			gb_def_dec_8(gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x26:{opcode:LD,bytes:2,cycles:[8],operands:[{name:H,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x26:
		{
			// LD H,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_hl.r8.h, gb_cpu_operand.r8.high);
			gb_def_cycles(8);
			break;
		}
		//0x27:{opcode:DAA,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:Z,N:-,H:0,C:C}}
		case 0x27:
		{
			unsigned long temp = gb_reg_af.r8.a;

			if ((gb_reg_af.r8.f & 0x40) == 0x40)
			{
				if ((gb_reg_af.r8.f & 0x20) == 0x20)
				{
					temp = (temp - 0x06) & 0xFF;
				}
			
				if ((gb_reg_af.r8.f & 0x10) == 0x10)
				{
					temp = temp - 0x60;
				}
			}
			else
			{
				if ((gb_reg_af.r8.f & 0x20) == 0x20 || (temp & 0x0F) > 0x09)
				{
					temp = temp + 0x06;
				}
			
				if ((gb_reg_af.r8.f & 0x10) == 0x10 || temp > 0x9F)
				{
					temp = temp + 0x60;
				}
			}

			if ((temp & 0x0100) == 0x0100)
			{
				gb_def_flag_c_set();
			}

			gb_reg_af.r8.a = (unsigned char)temp;
			
			if (gb_reg_af.r8.a == 0x00)
			{
				gb_def_flag_z_set();
			}
			else
			{
				gb_def_flag_z_clr();
			}

			gb_def_flag_h_clr();

			gb_def_cycles(4);
			break;
		}
		//0x28:{opcode:JR,bytes:2,cycles:[12,8],operands:[{name:Z,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x28:
		{
			// JRZ N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x80)
			{
				gb_reg_pc.r16 = (unsigned short)((unsigned short)gb_reg_pc.r16 + (signed char)gb_cpu_operand.r8.low);
				gb_def_cycles(12);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0x29:{opcode:ADD,bytes:1,cycles:[8],operands:[{name:HL,imm:true},{name:HL,imm:true}],imm:true,flags:{Z:-,N:0,H:H,C:C}}
		case 0x29:
		{
			// ADD HL,HL
			gb_def_add_16(gb_reg_hl.r16, gb_reg_hl.r16);
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_16();
			gb_def_cycles(8);
			break;
		}
		//0x2A:{opcode:LD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,inc:true,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x2A:
		{
			// LD A,[HL+]
			gb_def_read_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_reg_hl.r16 = (unsigned short)((unsigned short)gb_reg_hl.r16 + 1);
			gb_def_cycles(8);
			break;
		}
		//0x2B:{opcode:DEC,bytes:1,cycles:[8],operands:[{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x2B:
		{
			// DEC HL
			gb_def_dec_16(gb_reg_hl.r16);
			gb_def_cycles(8);
			break;
		}
		//0x2C:{opcode:INC,bytes:1,cycles:[4],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x2C:
		{
			// INC L
			gb_def_inc_8(gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x2D:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x2D:
		{
			// DEC L
			gb_def_dec_8(gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x2E:{opcode:LD,bytes:2,cycles:[8],operands:[{name:L,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x2E:
		{
			// LD E,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_hl.r8.l, gb_cpu_operand.r8.low);
			gb_def_cycles(8);
			break;
		}
		//0x2F:{opcode:CPL,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:1,H:1,C:-}}
		case 0x2F:
		{
			// CPL
			gb_reg_af.r8.a = (unsigned char)((unsigned char)gb_reg_af.r8.a ^ 0xFF);
			gb_def_flag_n_set();
			gb_def_flag_h_set();
			gb_def_cycles(4);
			break;
		}
		//0x30:{opcode:JR,bytes:2,cycles:[12,8],operands:[{name:NC,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x30:
		{
			// JRNC N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x00)
			{
				gb_reg_pc.r16 = (unsigned short)((unsigned short)gb_reg_pc.r16 + (signed char)gb_cpu_operand.r8.low);
				gb_def_cycles(12);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0x31:{opcode:LD,bytes:3,cycles:[12],operands:[{name:SP,imm:true},{name:n16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x31:
		{
			// LD SP,NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_16(gb_reg_sp.r16, gb_cpu_operand.r16);
			gb_def_cycles(12);
			break;
		}
		//0x32:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,dec:true,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x32:
		{
			// LD [HL-],A
			gb_def_write_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_reg_hl.r16 = (unsigned short)((unsigned short)gb_reg_hl.r16 - 1);
			gb_def_cycles(8);
			break;
		}
		//0x33:{opcode:INC,bytes:1,cycles:[8],operands:[{name:SP,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x33:
		{
			// INC SP
			gb_def_inc_16(gb_reg_sp.r16);
			gb_def_cycles(8);
			break;
		}
		//0x34:{opcode:INC,bytes:1,cycles:[12],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x34:
		{
			// INC [HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_result);
			gb_cpu_result = (unsigned char)((unsigned char)gb_cpu_result + 1);
			gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_result);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_reg_af.r8.f &= 0xD0;
			gb_reg_af.r8.f |= ((gb_cpu_result & 0x0F) == 0x00 ? 0x20 : 0x00);
			gb_def_cycles(12);
			break;
		}
		//0x35:{opcode:DEC,bytes:1,cycles:[12],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x35:
		{
			// DEC [HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_result);
			gb_cpu_result = (unsigned char)((unsigned char)gb_cpu_result - 1);
			gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_result);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_reg_af.r8.f &= 0xD0;
			gb_reg_af.r8.f |= ((gb_cpu_result & 0x0F) == 0x0F ? 0x20 : 0x00);
			gb_def_cycles(12);
			break;
		}
		//0x36:{opcode:LD,bytes:2,cycles:[12],operands:[{name:HL,imm:false},{name:n8,bytes:1,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x36:
		{
			// LD [HL],N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_result);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_result);
			gb_def_cycles(12);
			break;
		}		
		//0x37:{opcode:SCF,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:0,H:0,C:1}}
		case 0x37:
		{
			// SCF
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_set();
			gb_def_cycles(4);
			break;
		}
		//0x38:{opcode:JR,bytes:2,cycles:[12,8],operands:[{name:C,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x38:
		{
			// JRC N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x10)
			{
				gb_reg_pc.r16 = (unsigned short)((unsigned short)gb_reg_pc.r16 + (signed char)gb_cpu_operand.r8.low);
				gb_def_cycles(12);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0x39:{opcode:ADD,bytes:1,cycles:[8],operands:[{name:HL,imm:true},{name:SP,imm:true}],imm:true,flags:{Z:-,N:0,H:H,C:C}}
		case 0x39:
		{	
			// ADD HL,SP
			gb_def_add_16(gb_reg_hl.r16, gb_reg_sp.r16);
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_16();
			gb_def_cycles(8);
			break;
		}
		//0x3A:{opcode:LD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,dec:true,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x3A:
		{
			// LD A,[HL-]
			gb_def_read_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_reg_hl.r16 = (unsigned short)((unsigned short)gb_reg_hl.r16 - 1);
			gb_def_cycles(8);
			break;
		}
		//0x3B:{opcode:DEC,bytes:1,cycles:[8],operands:[{name:SP,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x3B:
		{
			// DEC SP
			gb_def_dec_16(gb_reg_sp.r16);
			gb_def_cycles(8);
			break;
		}
		//0x3C:{opcode:INC,bytes:1,cycles:[4],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:-}}
		case 0x3C:
		{
			// INC A
			gb_def_inc_8(gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_cycles(4);
			break;
		}
		//0x3D:{opcode:DEC,bytes:1,cycles:[4],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x3D:
		{
			// DEC A
			gb_def_dec_8(gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0x3E:{opcode:LD,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x3E:
		{	
			// LD A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_ld_8(gb_reg_af.r8.a, gb_cpu_operand.r8.low);
			gb_def_cycles(8);
			break;
		}
		//0x3F:{opcode:CCF,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:0,H:0,C:C}}
		case 0x3F:
		{
			// CCF
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_reg_af.r8.f = (unsigned char)((unsigned char)gb_reg_af.r8.f ^ 0x10);
			break;
		}


		//0x40:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x40:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x41:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x41:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x42:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x42:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x43:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x43:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x44:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x44:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x45:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x45:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x46:{opcode:LD,bytes:1,cycles:[8],operands:[{name:B,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x46:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_bc.r8.b);
			gb_def_cycles(8);
			break;
		}
		//0x47:{opcode:LD,bytes:1,cycles:[4],operands:[{name:B,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x47:
		{
			gb_def_ld_8(gb_reg_bc.r8.b, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x48:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x48:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x49:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x49:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x4A:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4A:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x4B:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4B:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x4C:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4C:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x4D:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4D:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x4E:{opcode:LD,bytes:1,cycles:[8],operands:[{name:C,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4E:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_bc.r8.c);
			gb_def_cycles(8);
			break;
		}
		//0x4F:{opcode:LD,bytes:1,cycles:[4],operands:[{name:C,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x4F:
		{
			gb_def_ld_8(gb_reg_bc.r8.c, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x50:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x50:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x51:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x51:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x52:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x52:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x53:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x53:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x54:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x54:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x55:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x55:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x56:{opcode:LD,bytes:1,cycles:[8],operands:[{name:D,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x56:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_de.r8.d);
			gb_def_cycles(8);
			break;
		}
		//0x57:{opcode:LD,bytes:1,cycles:[4],operands:[{name:D,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x57:
		{
			gb_def_ld_8(gb_reg_de.r8.d, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x58:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x58:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x59:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x59:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x5A:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5A:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x5B:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5B:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x5C:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5C:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x5D:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5D:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x5E:{opcode:LD,bytes:1,cycles:[8],operands:[{name:E,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5E:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_de.r8.e);
			gb_def_cycles(8);
			break;
		}
		//0x5F:{opcode:LD,bytes:1,cycles:[4],operands:[{name:E,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x5F:
		{
			gb_def_ld_8(gb_reg_de.r8.e, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x60:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x60:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x61:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x61:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x62:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x62:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x63:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x63:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x64:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x64:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x65:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x65:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x66:{opcode:LD,bytes:1,cycles:[8],operands:[{name:H,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x66:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_hl.r8.h);
			gb_def_cycles(8);
			break;
		}
		//0x67:{opcode:LD,bytes:1,cycles:[4],operands:[{name:H,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x67:
		{
			gb_def_ld_8(gb_reg_hl.r8.h, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x68:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x68:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x69:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x69:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x6A:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6A:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x6B:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6B:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x6C:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6C:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x6D:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6D:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x6E:{opcode:LD,bytes:1,cycles:[8],operands:[{name:L,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6E:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_hl.r8.l);
			gb_def_cycles(8);
			break;
		}
		//0x6F:{opcode:LD,bytes:1,cycles:[4],operands:[{name:L,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x6F:
		{
			gb_def_ld_8(gb_reg_hl.r8.l, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}
		//0x70:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:B,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x70:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_bc.r8.b);
			gb_def_cycles(8);
			break;
		}
		//0x71:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:C,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x71:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_bc.r8.c);
			gb_def_cycles(8);
			break;
		}
		//0x72:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:D,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x72:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_de.r8.d);
			gb_def_cycles(8);
			break;
		}
		//0x73:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:E,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x73:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_de.r8.e);
			gb_def_cycles(8);
			break;
		}
		//0x74:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:H,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x74:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_hl.r8.h);
			gb_def_cycles(8);
			break;
		}
		//0x75:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:L,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x75:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_hl.r8.l);
			gb_def_cycles(8);
			break;
		}
		//0x76:{opcode:HALT,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x76:
		{
			// HALT
			gb_cpu_halt = 0x01;
			gb_def_cycles(4);
			break;
		}
		//0x77:{opcode:LD,bytes:1,cycles:[8],operands:[{name:HL,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x77:
		{
			gb_def_write_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x78:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x78:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_cycles(4);
			break;
		}
		//0x79:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x79:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_cycles(4);
			break;
		}
		//0x7A:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7A:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_cycles(4);
			break;
		}
		//0x7B:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7B:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_cycles(4);
			break;
		}
		//0x7C:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7C:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_cycles(4);
			break;
		}
		//0x7D:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7D:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_cycles(4);
			break;
		}
		//0x7E:{opcode:LD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7E:
		{
			gb_def_read_8(gb_reg_hl.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0x7F:{opcode:LD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0x7F:
		{
			gb_def_ld_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_cycles(4);
			break;
		}


		//0x80:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x80:
		{
			// ADD A, B
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x81:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x81:
		{
			// ADD A, C
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x82:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x82:
		{
			// ADD A, D
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x83:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x83:
		{
			// ADD A, E
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x84:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x84:
		{
			// ADD A, H
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x85:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x85:
		{
			// ADD A, L
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x86:{opcode:ADD,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x86:
		{
			// ADD A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_add_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0x87:{opcode:ADD,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x87:
		{
			// ADD A, A
			gb_def_add_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x88:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x88:
		{
			// ADC A, B
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x89:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x89:
		{
			// ADC A, C
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x8A:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8A:
		{
			// ADC A, D
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x8B:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8B:
		{
			// ADC A, E
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x8C:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8C:
		{
			// ADC A, H
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x8D:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8D:
		{
			// ADC A, L
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x8E:{opcode:ADC,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8E:
		{
			// ADC A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_adc_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0x8F:{opcode:ADC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0x8F:
		{
			// ADC A, A
			gb_def_adc_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x90:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x90:
		{
			// SUB A, B
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x91:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x91:
		{
			// SUB A, C
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x92:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x92:
		{
			// SUB A, D
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x93:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x93:
		{
			// SUB A, E
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x94:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x94:
		{
			// SUB A, H
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x95:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x95:
		{
			// SUB A, L
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x96:{opcode:SUB,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x96:
		{
			// SUB A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_sub_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0x97:{opcode:SUB,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:1,N:1,H:0,C:0}}
		case 0x97:
		{
			// SUB A, A
			gb_def_sub_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_set();
			gb_def_flag_n_set();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0x98:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x98:
		{
			// SBC A, B
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x99:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x99:
		{
			// SBC A, C
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x9A:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x9A:
		{
			// SBC A, D
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x9B:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x9B:
		{
			// SBC A, E
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x9C:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x9C:
		{
			// SBC A, H
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x9D:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x9D:
		{
			// SBC A, L
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0x9E:{opcode:SBC,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:1,H:H,C:C}}
		case 0x9E:
		{
			// SBC A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_sbc_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0x9F:{opcode:SBC,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:-}}
		case 0x9F:
		{
			// SBC A, A
			gb_def_sbc_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_cycles(4);
			break;
		}
		//0xA0:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA0:
		{
			// AND A, B
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA1:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA1:
		{
			// AND A, C
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA2:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA2:
		{
			// AND A, D
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA3:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA3:
		{
			// AND A, E
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA4:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA4:
		{
			// AND A, H
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA5:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA5:
		{
			// AND A, L
			gb_def_and_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA6:{opcode:AND,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA6:
		{
			// AND A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_and_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xA7:{opcode:AND,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xA7:
		{
			// AND A, A
			//gb_def_and_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			//gb_def_flag_z_calc_8();

			gb_def_flag_z_clr();
			if (gb_reg_af.r8.a == 0x00) gb_def_flag_z_set();

			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA8:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xA8:
		{
			// XOR A, B
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xA9:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xA9:
		{
			// XOR A, C
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xAA:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xAA:
		{
			// XOR A, D
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xAB:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xAB:
		{
			// XOR A, E
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xAC:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xAC:
		{
			// XOR A, H
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xAD:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xAD:
		{
			// XOR A, L
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xAE:{opcode:XOR,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xAE:
		{
			// XOR A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_xor_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xAF:{opcode:XOR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:1,N:0,H:0,C:0}}
		case 0xAF:
		{
			// XOR A, A
			gb_def_xor_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_set();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB0:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB0:
		{
			// OR A, B
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB1:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB1:
		{
			// OR A, C
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB2:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB2:
		{
			// OR A, D
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB3:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB3:
		{
			// OR A, E
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB4:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB4:
		{
			// OR A, H
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB5:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB5:
		{
			// OR A, L
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB6:{opcode:OR,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB6:
		{
			// OR A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_or_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xB7:{opcode:OR,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xB7:
		{
			// OR A, A
			gb_def_or_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}
		//0xB8:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xB8:
		{
			// CP A, B
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_bc.r8.b);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xB9:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xB9:
		{
			// CP A, C
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_bc.r8.c);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xBA:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xBA:
		{
			// CP A, D
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_de.r8.d);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xBB:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xBB:
		{
			// CP A, E
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_de.r8.e);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xBC:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xBC:
		{
			// CP A, H
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_hl.r8.h);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xBD:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xBD:
		{
			// CP A, L
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_hl.r8.l);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(4);
			break;
		}
		//0xBE:{opcode:CP,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xBE:
		{
			// CP A,[HL]
			gb_def_read_8(gb_reg_hl.r16, gb_cpu_storage);
			gb_def_cp_8(gb_reg_af.r8.a, (unsigned char)gb_cpu_storage);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xBF:{opcode:CP,bytes:1,cycles:[4],operands:[{name:A,imm:true},{name:A,imm:true}],imm:true,flags:{Z:1,N:1,H:0,C:0}}
		case 0xBF:
		{
			// CP A, A
			gb_def_cp_8(gb_reg_af.r8.a, gb_reg_af.r8.a);
			gb_def_flag_z_set();
			gb_def_flag_n_set();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(4);
			break;
		}



		//0xC0:{opcode:RET,bytes:1,cycles:[20,8],operands:[{name:NZ,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC0:
		{
			// RET NZ
			if ((gb_reg_af.r8.f & 0x80) == 0x00)
			{
				gb_def_pop_16(gb_reg_pc.r16);
				gb_def_cycles(20);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0xC1:{opcode:POP,bytes:1,cycles:[12],operands:[{name:BC,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC1:
		{
			// POP BC
			gb_def_pop_16(gb_reg_bc.r16);
			gb_def_cycles(12);
			break;
		}
		//0xC2:{opcode:JP,bytes:3,cycles:[16,12],operands:[{name:NZ,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC2:
		{
			// JPNZ NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x00)
			{
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(16);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xC3:{opcode:JP,bytes:3,cycles:[16],operands:[{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC3:
		{
			// JP NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
			gb_def_cycles(16);
			break;
		}
		//0xC4:{opcode:CALL,bytes:3,cycles:[24,12],operands:[{name:NZ,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC4:
		{
			// CALLNZ NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x00)
			{
				gb_def_push_16(gb_reg_pc.r16);
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(24);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xC5:{opcode:PUSH,bytes:1,cycles:[16],operands:[{name:BC,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC5:
		{
			// PUSH BC
			gb_def_push_16(gb_reg_bc.r16);
			gb_def_cycles(16);
			break;
		}
		//0xC6:{opcode:ADD,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0xC6:
		{
			// ADD A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_add_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xC7:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$00,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC7:
		{
			// RST $00
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0000;
			gb_def_cycles(16);
			break;
		}
		//0xC8:{opcode:RET,bytes:1,cycles:[20,8],operands:[{name:Z,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC8:
		{
			// RET Z
			if ((gb_reg_af.r8.f & 0x80) == 0x80)
			{
				gb_def_pop_16(gb_reg_pc.r16);
				gb_def_cycles(20);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0xC9:{opcode:RET,bytes:1,cycles:[16],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xC9:
		{
			// RET
			gb_def_pop_16(gb_reg_pc.r16);
			gb_def_cycles(16);
			break;
		}
		//0xCA:{opcode:JP,bytes:3,cycles:[16,12],operands:[{name:Z,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xCA:
		{
			// JPZ NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x80)
			{
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(16);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xCB Prefix - See Below
		//0xCC:{opcode:CALL,bytes:3,cycles:[24,12],operands:[{name:Z,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xCC:
		{
			// CALLZ NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x80) == 0x80)
			{
				gb_def_push_16(gb_reg_pc.r16);
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(24);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xCD:{opcode:CALL,bytes:3,cycles:[24],operands:[{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xCD:
		{
			// CALL NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
			gb_def_cycles(24);
			break;
		}
		//0xCE:{opcode:ADC,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:0,H:H,C:C}}
		case 0xCE:
		{
			// ADC A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_adc_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xCF:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$08,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xCF:
		{
			// RST $08
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0008;
			gb_def_cycles(16);
			break;
		}
		//0xD0:{opcode:RET,bytes:1,cycles:[20,8],operands:[{name:NC,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD0:
		{
			// RET NC
			if ((gb_reg_af.r8.f & 0x10) == 0x00)
			{
				gb_def_pop_16(gb_reg_pc.r16);
				gb_def_cycles(20);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0xD1:{opcode:POP,bytes:1,cycles:[12],operands:[{name:DE,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD1:
		{
			// POP DE
			gb_def_pop_16(gb_reg_de.r16);
			gb_def_cycles(12);
			break;
		}
		//0xD2:{opcode:JP,bytes:3,cycles:[16,12],operands:[{name:NC,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD2:
		{
			// JPNC NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x00)
			{
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(16);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xD3:{opcode:ILLEGAL_D3,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD3:
		{
			gb_def_cycles(4);
			// add code here		
			break;
		}
		//0xD4:{opcode:CALL,bytes:3,cycles:[24,12],operands:[{name:NC,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD4:
		{
			// CALLNC NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x00)
			{
				gb_def_push_16(gb_reg_pc.r16);
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(24);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xD5:{opcode:PUSH,bytes:1,cycles:[16],operands:[{name:DE,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD5:
		{
			// PUSH DE
			gb_def_push_16(gb_reg_de.r16);
			gb_def_cycles(16);
			break;
		}
		//0xD6:{opcode:SUB,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xD6:
		{
			// SUB A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_sub_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xD7:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$10,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD7:
		{
			// RST $10
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0010;
			gb_def_cycles(16);
			break;
		}
		//0xD8:{opcode:RET,bytes:1,cycles:[20,8],operands:[{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD8:
		{
			// RET C
			if ((gb_reg_af.r8.f & 0x10) == 0x10)
			{
				gb_def_pop_16(gb_reg_pc.r16);
				gb_def_cycles(20);
			}
			else
			{
				gb_def_cycles(8);
			}
			break;
		}
		//0xD9:{opcode:RETI,bytes:1,cycles:[16],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xD9:
		{
			// RETI
			gb_def_pop_16(gb_reg_pc.r16);
			gb_cpu_ime = 1;
			gb_def_cycles(16);
			break;
		}
		//0xDA:{opcode:JP,bytes:3,cycles:[16,12],operands:[{name:C,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xDA:
		{
			// JPC NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x10)
			{
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(16);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xDB:{opcode:ILLEGAL_DB,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xDB:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xDC:{opcode:CALL,bytes:3,cycles:[24,12],operands:[{name:C,imm:true},{name:a16,bytes:2,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xDC:
		{
			// CALLC NN
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			if ((gb_reg_af.r8.f & 0x10) == 0x10)
			{
				gb_def_push_16(gb_reg_pc.r16);
				gb_reg_pc.r16 = (unsigned short)gb_cpu_operand.r16;
				gb_def_cycles(24);
			}
			else
			{
				gb_def_cycles(12);
			}
			break;
		}
		//0xDD:{opcode:ILLEGAL_DD,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xDD:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xDE:{opcode:SBC,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xDE:
		{
			// SBC A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_sbc_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xDF:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$18,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xDF:
		{
			// RST $18
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0018;
			gb_def_cycles(16);
			break;
		}
		//0xE0:{opcode:LDH,bytes:2,cycles:[12],operands:[{name:a8,bytes:1,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE0:
		{
			// LDH [N],A
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_cpu_operand.r8.high = (unsigned char)0xFF;
			gb_def_write_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(12);
			break;
		}
		//0xE1:{opcode:POP,bytes:1,cycles:[12],operands:[{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE1:
		{
			// POP HL
			gb_def_pop_16(gb_reg_hl.r16);
			gb_def_cycles(12);
			break;
		}
		//0xE2:{opcode:LDH,bytes:1,cycles:[8],operands:[{name:C,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE2:
		{
			// LDH [C],A
			gb_cpu_operand.r8.low = (unsigned char)gb_reg_bc.r8.c;
			gb_cpu_operand.r8.high = (unsigned char)0xFF;
			gb_def_write_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(8);
			break;
		}
		//0xE3:{opcode:ILLEGAL_E3,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE3:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xE4:{opcode:ILLEGAL_E4,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE4:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xE5:{opcode:PUSH,bytes:1,cycles:[16],operands:[{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE5:
		{
			// PUSH HL
			gb_def_push_16(gb_reg_hl.r16);
			gb_def_cycles(16);
			break;
		}
		//0xE6:{opcode:AND,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:0}}
		case 0xE6:
		{
			// AND A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_and_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_set();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xE7:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$20,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE7:
		{
			// RST $20
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0020;
			gb_def_cycles(16);
			break;
		}
		//0xE8:{opcode:ADD,bytes:2,cycles:[16],operands:[{name:SP,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:0,N:0,H:H,C:C}}
		case 0xE8:
		{
			// ADD SP,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f |= (((gb_reg_sp.r16 & 0x0F) + (gb_cpu_operand.r8.low & 0x0F) > 0x0F) ? 0x20 : 0x00);
			gb_reg_af.r8.f |= (((gb_reg_sp.r16 & 0xFF) + (gb_cpu_operand.r8.low & 0xFF) > 0xFF) ? 0x10 : 0x00);
			gb_reg_sp.r16 = (unsigned short)(gb_reg_sp.r16 + (signed char)gb_cpu_operand.r8.low);

			gb_def_cycles(16);
			break;
		}
		//0xE9:{opcode:JP,bytes:1,cycles:[4],operands:[{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xE9:
		{
			// JP HL
			gb_reg_pc.r16 = (unsigned short)gb_reg_hl.r16;
			gb_def_cycles(4);
			break;
		}
		//0xEA:{opcode:LD,bytes:3,cycles:[16],operands:[{name:a16,bytes:2,imm:false},{name:A,imm:true}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xEA:
		{
			// LD [NN],A
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);	
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_write_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(16);
			break;
		}
		//0xEB:{opcode:ILLEGAL_EB,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xEB:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xEC:{opcode:ILLEGAL_EC,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xEC:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xED:{opcode:ILLEGAL_ED,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xED:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xEE:{opcode:XOR,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xEE:
		{
			// XOR A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_xor_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xEF:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$28,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xEF:
		{
			// RST $28
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0028;
			gb_def_cycles(16);
			break;
		}
		//0xF0:{opcode:LDH,bytes:2,cycles:[12],operands:[{name:A,imm:true},{name:a8,bytes:1,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF0:
		{
			// LDH A,[N]
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_cpu_operand.r8.high = (unsigned char)0xFF;
			gb_def_read_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(12);
			break;
		}
		//0xF1:{opcode:POP,bytes:1,cycles:[12],operands:[{name:AF,imm:true}],imm:true,flags:{Z:Z,N:N,H:H,C:C}}
		case 0xF1:
		{
			// POP AF
			gb_reg_af.r8.f = gb_read(gb_reg_sp.r16);
			gb_reg_sp.r16 += 1;
			gb_reg_af.r8.f &= 0xF0;
			gb_reg_af.r8.a = gb_read(gb_reg_sp.r16);
			gb_reg_sp.r16 += 1;
			gb_cpu_cycles += 12;

			break;
		}
		//0xF2:{opcode:LDH,bytes:1,cycles:[8],operands:[{name:A,imm:true},{name:C,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF2:
		{
			// LDH A,[C]
			gb_cpu_operand.r8.low = (unsigned char)gb_reg_bc.r8.c;
			gb_cpu_operand.r8.high = (unsigned char)0xFF;
			gb_def_read_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(12);
			break;
		}
		//0xF3:{opcode:DI,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF3:
		{
			//printf("DI %04X\n", (unsigned short)gb_reg_pc.r16);

			// DI
			gb_cpu_ime = 0;
			gb_def_cycles(4);
			break;
		}
		//0xF4:{opcode:ILLEGAL_F4,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF4:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xF5:{opcode:PUSH,bytes:1,cycles:[16],operands:[{name:AF,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF5:
		{
			// PUSH AF
			gb_reg_sp.r16 -= 1;
			gb_write(gb_reg_sp.r16, gb_reg_af.r8.a);
			gb_reg_sp.r16 -= 1;
			gb_reg_af.r8.f &= 0xF0;
			gb_write(gb_reg_sp.r16, gb_reg_af.r8.f);
			gb_cpu_cycles += 16;

			break;
		}
		//0xF6:{opcode:OR,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
		case 0xF6:
		{
			// OR A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_or_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_clr();
			gb_def_flag_h_clr();
			gb_def_flag_c_clr();
			gb_def_cycles(8);
			break;
		}
		//0xF7:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$30,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF7:
		{
			// RST $30
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0030;
			gb_def_cycles(16);
			break;
		}
		//0xF8:{opcode:LD,bytes:2,cycles:[12],operands:[{name:HL,imm:true},{name:SP,inc:true,imm:true},{name:e8,bytes:1,imm:true}],imm:true,flags:{Z:0,N:0,H:H,C:C}}
		case 0xF8:
		{
			// LD HL,SP+N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_reg_hl.r16 = (unsigned short)(gb_reg_sp.r16 + (signed char)gb_cpu_operand.r8.low);
			gb_reg_af.r8.f = 0x00;
			gb_reg_af.r8.f |= (((gb_reg_sp.r16 & 0x0F) + (gb_cpu_operand.r8.low & 0x0F) > 0x0F) ? 0x20 : 0x00);
			gb_reg_af.r8.f |= (((gb_reg_sp.r16 & 0xFF) + (gb_cpu_operand.r8.low & 0xFF) > 0xFF) ? 0x10 : 0x00);

			gb_def_cycles(12);
			break;
		}
		//0xF9:{opcode:LD,bytes:1,cycles:[8],operands:[{name:SP,imm:true},{name:HL,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xF9:
		{
			// LD SP,HL
			gb_reg_sp.r16 = (unsigned short)gb_reg_hl.r16;
			gb_def_cycles(8);
			break;
		}
		//0xFA:{opcode:LD,bytes:3,cycles:[16],operands:[{name:A,imm:true},{name:a16,bytes:2,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
		case 0xFA:
		{
			// LD A,[NN]
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.low);
			gb_def_step(gb_reg_pc.r16, 1);	
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_read_8(gb_cpu_operand.r16, gb_reg_af.r8.a);
			gb_def_cycles(16);
			break;
		}
		//0xFB:{opcode:EI,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xFB:
		{
			//printf("EI %04X\n", (unsigned short)gb_reg_pc.r16);

			// EI
			gb_cpu_ime = 1;
			gb_def_cycles(4);
			break;
		}
		//0xFC:{opcode:ILLEGAL_FC,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xFC:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xFD:{opcode:ILLEGAL_FD,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xFD:
		{
			gb_def_cycles(4);
			// add code here
			break;
		}
		//0xFE:{opcode:CP,bytes:2,cycles:[8],operands:[{name:A,imm:true},{name:n8,bytes:1,imm:true}],imm:true,flags:{Z:Z,N:1,H:H,C:C}}
		case 0xFE:
		{
			// CP A,N
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_operand.r8.high);
			gb_def_step(gb_reg_pc.r16, 1);
			gb_def_cp_8(gb_reg_af.r8.a, gb_cpu_operand.r8.high);
			gb_def_flag_z_calc_8();
			gb_def_flag_n_set();
			gb_def_flag_c_calc_8();
			gb_def_cycles(8);
			break;
		}
		//0xFF:{opcode:RST,bytes:1,cycles:[16],operands:[{name:$38,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}}
		case 0xFF:
		{
			// RST $38
			gb_def_push_16(gb_reg_pc.r16);
			gb_reg_pc.r16 = (unsigned short)0x0038;
			gb_def_cycles(16);
			break;
		}
		
		//0xCB:{opcode:PREFIX,bytes:1,cycles:[4],operands:[],imm:true,flags:{Z:-,N:-,H:-,C:-}}
		case 0xCB:
		{
			gb_def_read_8(gb_reg_pc.r16, gb_cpu_opcode);

			gb_def_step(gb_reg_pc.r16, 1);

			// 0xCB Prefixed Codes

			switch ((gb_cpu_opcode & 0xC0))
			{
				case 0x00:
				{
					switch ((gb_cpu_opcode & 0x3F))
					{
						//0x00:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x00:
						{
							// RLC B
							gb_def_rlc_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x01:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x01:
						{
							// RLC C
							gb_def_rlc_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x02:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x02:
						{
							// RLC D
							gb_def_rlc_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x03:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x03:
						{
							// RLC E
							gb_def_rlc_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x04:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x04:
						{
							// RLC B
							gb_def_rlc_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x05:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x05:
						{
							// RLC L
							gb_def_rlc_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x06:{opcode:RLC,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x06:
						{
							// RLC [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_rlc_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x07:{opcode:RLC,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x07:
						{
							// RLC B
							gb_def_rlc_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x08:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x08:
						{
							// RRC B
							gb_def_rrc_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x09:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x09:
						{
							// RRC C
							gb_def_rrc_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x0A:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0A:
						{
							// RRC D
							gb_def_rrc_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x0B:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0B:
						{
							// RRC E
							gb_def_rrc_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x0C:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0C:
						{
							// RRC H
							gb_def_rrc_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x0D:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0D:
						{
							// RRC L
							gb_def_rrc_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x0E:{opcode:RRC,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0E:
						{
							// RRC [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_rrc_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x0F:{opcode:RRC,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x0F:
						{
							// RRC A
							gb_def_rrc_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x10:{opcode:RL,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x10:
						{
							// RL B
							gb_def_rl_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x11:{opcode:RL,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x11:
						{
							// RL C
							gb_def_rl_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x12:{opcode:RL,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x12:
						{
							// RL D
							gb_def_rl_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x13:{opcode:RL,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x13:
						{
							// RL E
							gb_def_rl_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x14:{opcode:RL,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x14:
						{
							// RL H
							gb_def_rl_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x15:{opcode:RL,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x15:
						{
							// RL L
							gb_def_rl_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x16:{opcode:RL,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x16:
						{
							// RL [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_rl_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x17:{opcode:RL,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x17:
						{
							// RL A
							gb_def_rl_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x18:{opcode:RR,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x18:
						{
							// RR B
							gb_def_rr_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x19:{opcode:RR,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x19:
						{
							// RR C
							gb_def_rr_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x1A:{opcode:RR,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1A:
						{
							// RR D
							gb_def_rr_8(gb_reg_de.r8.d);
							gb_def_cycles(8);
							break;
						}
						//0x1B:{opcode:RR,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1B:
						{
							// RR E
							gb_def_rr_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x1C:{opcode:RR,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1C:
						{
							// RR H
							gb_def_rr_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x1D:{opcode:RR,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1D:
						{
							// RR L
							gb_def_rr_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x1E:{opcode:RR,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1E:
						{
							// RR [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_rr_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x1F:{opcode:RR,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x1F:
						{
							// RR A
							gb_def_rr_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x20:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x20:
						{
							// SLA B
							gb_def_sla_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x21:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x21:
						{
							// SLA C
							gb_def_sla_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x22:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x22:
						{
							// SLA D
							gb_def_sla_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x23:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x23:
						{
							// SLA E
							gb_def_sla_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x24:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x24:
						{
							// SLA H
							gb_def_sla_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x25:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x25:
						{
							// SLA L
							gb_def_sla_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x26:{opcode:SLA,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x26:
						{
							// SLA [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_sla_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x27:{opcode:SLA,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x27:
						{
							// SLA A
							gb_def_sla_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x28:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x28:
						{
							// SRA B
							gb_def_sra_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x29:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x29:
						{
							// SRA C
							gb_def_sra_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x2A:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2A:
						{
							// SRA D
							gb_def_sra_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x2B:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2B:
						{
							// SRA E
							gb_def_sra_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x2C:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2C:
						{
							// SRA H
							gb_def_sra_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x2D:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2D:
						{
							// SRA L
							gb_def_sra_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x2E:{opcode:SRA,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2E:
						{
							// SRA [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_sra_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x2F:{opcode:SRA,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x2F:
						{
							// SRA A
							gb_def_sra_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
						//0x30:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x30:
						{
							// SWAP B
							gb_def_swap_8(gb_reg_bc.r8.b);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x31:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x31:
						{
							// SWAP C
							gb_def_swap_8(gb_reg_bc.r8.c);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x32:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x32:
						{
							// SWAP D
							gb_def_swap_8(gb_reg_de.r8.d);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x33:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x33:
						{
							// SWAP E
							gb_def_swap_8(gb_reg_de.r8.e);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x34:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x34:
						{
							// SWAP H
							gb_def_swap_8(gb_reg_hl.r8.h);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x35:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x35:
						{
							// SWAP L
							gb_def_swap_8(gb_reg_hl.r8.l);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x36:{opcode:SWAP,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x36:
						{
							// SWAP [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_swap_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(16);	
							break;
						}
						//0x37:{opcode:SWAP,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:0}}
						case 0x37:
						{
							// SWAP A
							gb_def_swap_8(gb_reg_af.r8.a);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_clr();
							gb_def_flag_c_clr();
							gb_def_cycles(8);	
							break;
						}
						//0x38:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x38:
						{
							// SRL B
							gb_def_srl_8(gb_reg_bc.r8.b);
							gb_def_cycles(8);	
							break;
						}
						//0x39:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x39:
						{
							// SRL C
							gb_def_srl_8(gb_reg_bc.r8.c);
							gb_def_cycles(8);	
							break;
						}
						//0x3A:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x3A:
						{
							// SRL D
							gb_def_srl_8(gb_reg_de.r8.d);
							gb_def_cycles(8);	
							break;
						}
						//0x3B:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x3B:
						{
							// SRL E
							gb_def_srl_8(gb_reg_de.r8.e);
							gb_def_cycles(8);	
							break;
						}
						//0x3C:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x3C:
						{
							// SRL H
							gb_def_srl_8(gb_reg_hl.r8.h);
							gb_def_cycles(8);	
							break;
						}
						//0x3D:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x3D:
						{
							// SRL L
							gb_def_srl_8(gb_reg_hl.r8.l);
							gb_def_cycles(8);	
							break;
						}
						//0x3E:{opcode:SRL,bytes:2,cycles:[16],operands:[{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:0,C:C}}
						case 0x3E:
						{
							// SRL [HL]
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_srl_8(gb_cpu_operand.r8.high);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);	
							break;
						}
						//0x3F:{opcode:SRL,bytes:2,cycles:[8],operands:[{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:0,C:C}}	
						case 0x3F:
						{
							// SRL A
							gb_def_srl_8(gb_reg_af.r8.a);
							gb_def_cycles(8);	
							break;
						}
					}	

					break;
				}
				case 0x40:
				{
					//0x40:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x41:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x42:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x43:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x44:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x45:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x46:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:0,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x47:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x48:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x49:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4A:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4B:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4C:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4D:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4E:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:1,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x4F:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x50:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x51:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x52:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x53:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x54:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x55:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x56:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:2,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x57:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x58:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x59:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5A:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5B:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5C:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5D:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5E:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:3,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x5F:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x60:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x61:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x62:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x63:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x64:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x65:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x66:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:4,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x67:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x68:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x69:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6A:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6B:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6C:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6D:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6E:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:5,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x6F:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x70:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x71:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x72:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x73:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x74:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x75:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x76:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:6,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x77:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x78:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:B,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x79:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:C,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7A:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:D,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7B:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:E,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7C:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:H,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7D:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:L,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7E:{opcode:BIT,bytes:2,cycles:[12],operands:[{name:7,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:Z,N:0,H:1,C:-}}
					//0x7F:{opcode:BIT,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:A,imm:true}],imm:true,flags:{Z:Z,N:0,H:1,C:-}}

					switch ((gb_cpu_opcode & 0x38))
					{
						case 0x00: { gb_cpu_bits = 0x01; break; }
						case 0x08: { gb_cpu_bits = 0x02; break; }
						case 0x10: { gb_cpu_bits = 0x04; break; }
						case 0x18: { gb_cpu_bits = 0x08; break; }
						case 0x20: { gb_cpu_bits = 0x10; break; }
						case 0x28: { gb_cpu_bits = 0x20; break; }
						case 0x30: { gb_cpu_bits = 0x40; break; }
						case 0x38: { gb_cpu_bits = 0x80; break; }
					}

					switch ((gb_cpu_opcode & 0x07))
					{
						case 0x00:
						{
							gb_def_bit_8(gb_reg_bc.r8.b, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x01:
						{
							gb_def_bit_8(gb_reg_bc.r8.c, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x02:
						{
							gb_def_bit_8(gb_reg_de.r8.d, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x03:
						{
							gb_def_bit_8(gb_reg_de.r8.e, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x04:
						{
							gb_def_bit_8(gb_reg_hl.r8.h, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x05:
						{
							gb_def_bit_8(gb_reg_hl.r8.l, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
						case 0x06:
						{
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_bit_8(gb_cpu_operand.r8.high, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(12);
							break;
						}
						case 0x07:
						{
							gb_def_bit_8(gb_reg_af.r8.a, gb_cpu_bits);
							gb_def_flag_z_calc_8();
							gb_def_flag_n_clr();
							gb_def_flag_h_set();
							gb_def_cycles(8);
							break;
						}
					}

					break;
				}
				case 0x80:
				{
					//0x80:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x81:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x82:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x83:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x84:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x85:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x86:{opcode:RES,bytes:2,cycles:[16],operands:[{name:0,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0x87:{opcode:RES,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x88:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x89:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x8A:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x8B:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x8C:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x8D:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x8E:{opcode:RES,bytes:2,cycles:[16],operands:[{name:1,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0x8F:{opcode:RES,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x90:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x91:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x92:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x93:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x94:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x95:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x96:{opcode:RES,bytes:2,cycles:[16],operands:[{name:2,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0x97:{opcode:RES,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x98:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x99:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x9A:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x9B:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x9C:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x9D:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0x9E:{opcode:RES,bytes:2,cycles:[16],operands:[{name:3,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0x9F:{opcode:RES,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA0:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA1:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA2:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA3:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA4:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA5:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA6:{opcode:RES,bytes:2,cycles:[16],operands:[{name:4,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xA7:{opcode:RES,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA8:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xA9:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xAA:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xAB:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xAC:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xAD:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xAE:{opcode:RES,bytes:2,cycles:[16],operands:[{name:5,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xAF:{opcode:RES,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB0:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB1:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB2:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB3:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB4:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB5:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB6:{opcode:RES,bytes:2,cycles:[16],operands:[{name:6,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xB7:{opcode:RES,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB8:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xB9:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xBA:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xBB:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xBC:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xBD:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xBE:{opcode:RES,bytes:2,cycles:[16],operands:[{name:7,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xBF:{opcode:RES,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}

					switch ((gb_cpu_opcode & 0x38))
					{
						case 0x00: { gb_cpu_bits = 0xFE; break; }
						case 0x08: { gb_cpu_bits = 0xFD; break; }
						case 0x10: { gb_cpu_bits = 0xFB; break; }
						case 0x18: { gb_cpu_bits = 0xF7; break; }
						case 0x20: { gb_cpu_bits = 0xEF; break; }
						case 0x28: { gb_cpu_bits = 0xDF; break; }
						case 0x30: { gb_cpu_bits = 0xBF; break; }
						case 0x38: { gb_cpu_bits = 0x7F; break; }
					}

					switch ((gb_cpu_opcode & 0x07))
					{
						case 0x00:
						{
							gb_def_res_8(gb_reg_bc.r8.b, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x01:
						{
							gb_def_res_8(gb_reg_bc.r8.c, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x02:
						{
							gb_def_res_8(gb_reg_de.r8.d, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x03:
						{
							gb_def_res_8(gb_reg_de.r8.e, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x04:
						{
							gb_def_res_8(gb_reg_hl.r8.h, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x05:
						{
							gb_def_res_8(gb_reg_hl.r8.l, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x06:
						{
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_res_8(gb_cpu_operand.r8.high, gb_cpu_bits);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);
							break;
						}
						case 0x07:
						{
							gb_def_res_8(gb_reg_af.r8.a, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
					}

					break;
				}
				case 0xC0:
				{
					//0xC0:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC1:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC2:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC3:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC4:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC5:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC6:{opcode:SET,bytes:2,cycles:[16],operands:[{name:0,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xC7:{opcode:SET,bytes:2,cycles:[8],operands:[{name:0,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC8:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xC9:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xCA:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xCB:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xCC:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xCD:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xCE:{opcode:SET,bytes:2,cycles:[16],operands:[{name:1,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xCF:{opcode:SET,bytes:2,cycles:[8],operands:[{name:1,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD0:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD1:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD2:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD3:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD4:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD5:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD6:{opcode:SET,bytes:2,cycles:[16],operands:[{name:2,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xD7:{opcode:SET,bytes:2,cycles:[8],operands:[{name:2,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD8:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xD9:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xDA:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xDB:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xDC:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xDD:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xDE:{opcode:SET,bytes:2,cycles:[16],operands:[{name:3,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xDF:{opcode:SET,bytes:2,cycles:[8],operands:[{name:3,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE0:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE1:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE2:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE3:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE4:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE5:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE6:{opcode:SET,bytes:2,cycles:[16],operands:[{name:4,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xE7:{opcode:SET,bytes:2,cycles:[8],operands:[{name:4,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE8:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xE9:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xEA:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xEB:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xEC:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xED:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xEE:{opcode:SET,bytes:2,cycles:[16],operands:[{name:5,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xEF:{opcode:SET,bytes:2,cycles:[8],operands:[{name:5,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF0:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF1:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF2:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF3:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF4:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF5:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF6:{opcode:SET,bytes:2,cycles:[16],operands:[{name:6,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xF7:{opcode:SET,bytes:2,cycles:[8],operands:[{name:6,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF8:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:B,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xF9:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:C,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xFA:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:D,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xFB:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:E,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xFC:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:H,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xFD:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:L,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}
					//0xFE:{opcode:SET,bytes:2,cycles:[16],operands:[{name:7,imm:true},{name:HL,imm:false}],imm:false,flags:{Z:-,N:-,H:-,C:-}}
					//0xFF:{opcode:SET,bytes:2,cycles:[8],operands:[{name:7,imm:true},{name:A,imm:true}],imm:true,flags:{Z:-,N:-,H:-,C:-}}	

					switch ((gb_cpu_opcode & 0x38))
					{
						case 0x00: { gb_cpu_bits = 0x01; break; }
						case 0x08: { gb_cpu_bits = 0x02; break; }
						case 0x10: { gb_cpu_bits = 0x04; break; }
						case 0x18: { gb_cpu_bits = 0x08; break; }
						case 0x20: { gb_cpu_bits = 0x10; break; }
						case 0x28: { gb_cpu_bits = 0x20; break; }
						case 0x30: { gb_cpu_bits = 0x40; break; }
						case 0x38: { gb_cpu_bits = 0x80; break; }
					}

					switch ((gb_cpu_opcode & 0x07))
					{
						case 0x00:
						{
							gb_def_set_8(gb_reg_bc.r8.b, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x01:
						{
							gb_def_set_8(gb_reg_bc.r8.c, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x02:
						{
							gb_def_set_8(gb_reg_de.r8.d, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x03:
						{
							gb_def_set_8(gb_reg_de.r8.e, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x04:
						{
							gb_def_set_8(gb_reg_hl.r8.h, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x05:
						{
							gb_def_set_8(gb_reg_hl.r8.l, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
						case 0x06:
						{
							gb_def_read_8(gb_reg_hl.r16, gb_cpu_operand.r8.high);
							gb_def_set_8(gb_cpu_operand.r8.high, gb_cpu_bits);
							gb_def_write_8(gb_reg_hl.r16, (unsigned char)gb_cpu_operand.r8.high);
							gb_def_cycles(16);
							break;
						}
						case 0x07:
						{
							gb_def_set_8(gb_reg_af.r8.a, gb_cpu_bits);
							gb_def_cycles(8);
							break;
						}
					}				

					break;
				}
			}

			break;
		}		
	}
}

// draws a single line
void gb_line()
{
	// disabled
	if ((gb_io_lcdc & 0x80) == 0x00) return;

	unsigned long loc, start, end, tile, left, right, shift, pos, pal, val, color, obj;
	signed int spr;
	unsigned short pri_enable, pri_value;
	
	unsigned long line = gb_io_ly * 160;

	// background
	loc = 0x1800 + (((gb_io_ly + gb_io_scy) & 0xF8) << 2);

	if ((gb_io_lcdc & 0x08) == 0x08) loc += 0x0400;

	start = ((gb_io_scx & 0xF8) >> 3);
	end = start + 21;

	for (unsigned long x=start; x<end; x++)
	{
		tile = (gb_mem_vram[loc + (x & 0x1F)] << 4);

		if (tile < 2048 && (gb_io_lcdc & 0x10) == 0x00) tile += 0x1000;

		tile += (((gb_io_ly + gb_io_scy) & 0x07) << 1);

		left = gb_mem_vram[tile];
		right = (gb_mem_vram[tile+1] << 1);
		
		for (signed long i=7; i>=0; i--)
		{
			shift = (x*8+i) - (gb_io_scx);
			
			if ((shift & 0x00FF) < 160)
			{
				pos = line + shift;
				pal = ((((left & 0x01)) | ((right & 0x02))) << 1);
				val = ((gb_io_bgp & (0x03 << pal)) >> pal);

				// pre-palette
				gb_game_screen_buffer[pos] = val;
			}

			left = (left >> 1);
			right = (right >> 1);
		}
	}

	// window
	if ((gb_io_lcdc & 0x20) == 0x20)
	{
		if (gb_io_ly >= gb_io_wy && gb_io_ly < (gb_io_wy + 144))
		{
			loc = 0x1800 + (((gb_io_ly - gb_io_wy) & 0xF8) << 2);

			if ((gb_io_lcdc & 0x40) == 0x40) loc += 0x0400;

			start = 0;
			end = 21;

			for (unsigned long x=start; x<end; x++)
			{
				tile = (gb_mem_vram[loc + (x & 0x1F)] << 4);

				if (tile < 2048 && (gb_io_lcdc & 0x10) == 0x00) tile += 0x1000;

				tile += (((gb_io_ly - gb_io_wy) & 0x07) << 1);

				left = gb_mem_vram[tile];
				right = (gb_mem_vram[tile+1] << 1);
				
				for (signed int i=7; i>=0; i--)
				{
					shift = (x*8+i) + (gb_io_wx-7);
					
					if ((shift & 0x00FF) < 160)
					{
						pos = line + shift;
						pal = ((((left & 0x01)) | ((right & 0x02))) << 1);
						val = ((gb_io_bgp & (0x03 << pal)) >> pal);
						
						// pre-palette
						gb_game_screen_buffer[pos] = val;
					}

					left = (left >> 1);
					right = (right >> 1);
				}
			}
		}
	}

	// objects
	if ((gb_io_lcdc & 0x02) == 0x02)
	{
		if ((gb_io_lcdc & 0x04) == 0x00) // 8x8 objects
		{
			for (signed int i=156; i>=0; i-=4)
			{
				spr = (signed int)(gb_io_ly - gb_mem_oam[i] + 16);
				
				if (spr >= 0 && spr < 8)
				{
					if ((gb_mem_oam[i+3] & 0x80) == 0x80)
					{
						pri_enable = 1;
						pri_value = (unsigned short)(gb_io_bgp & 0x03);
					}
					else
					{
						pri_enable = 0;
					}

					tile = (gb_mem_oam[i+2] << 4);

					if ((gb_mem_oam[i+3] & 0x10) == 0x00)
					{
						loc = gb_io_obp0;
						obj = 0x04;
					}
					else
					{
						loc = gb_io_obp1;
						obj = 0x08;
					}

					switch ((gb_mem_oam[i+3] & 0x60))
					{
						case 0x00: // no flips
						{
							tile += (spr << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] + j - 8;
								
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}
							
							break;
						}
						case 0x20: // X-flip
						{
							tile += (spr << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] - j - 1;
							
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);
								
									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
						case 0x40: // Y-flip
						{
							tile += ((7-spr) << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] + j - 8;
								
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
									
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
						case 0x60: // X-flip and Y-flip
						{
							tile += ((7-spr) << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] - j - 1;
							
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
					}
				}
			}					
		}
		else // 8x16 objects
		{
			for (signed int i=156; i>=0; i-=4)
			{
				spr = (signed int)(gb_io_ly - gb_mem_oam[i] + 16);

				if (spr >= 0 && spr < 16)
				{
					if ((gb_mem_oam[i+3] & 0x80) == 0x80)
					{
						pri_enable = 1;
						pri_value = (unsigned short)(gb_io_bgp & 0x03);
					}
					else
					{
						pri_enable = 0;
					}

					tile = (gb_mem_oam[i+2] << 4);

					if ((gb_mem_oam[i+3] & 0x10) == 0x00)
					{
						loc = gb_io_obp0;
						obj = 0x04;
					}
					else
					{
						loc = gb_io_obp1;
						obj = 0x08;
					}

					switch ((gb_mem_oam[i+3] & 0x60))
					{
						case 0x00: // no flips
						{
							tile += (spr << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] + j - 8;
								
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}
							
							break;
						}
						case 0x20: // X-flip
						{
							tile += (spr << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] - j - 1;
							
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);
								
									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
						case 0x40: // Y-flip
						{
							tile += ((15-spr) << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] + j - 8;
								
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
						case 0x60: // X-flip and Y-flip
						{
							tile += ((15-spr) << 1);

							left = gb_mem_vram[tile];
							right = (gb_mem_vram[tile+1] << 1);
			
							for (signed int j=7; j>=0; j--)
							{
								shift = gb_mem_oam[i+1] - j - 1;
							
								if ((shift & 0x00FF) < 160)
								{
									pos = line + shift;
									pal = ((((left & 0x01)) | ((right & 0x02))) << 1);

									if (pal != 0x00)
									{
										val = ((loc & (0x03 << pal)) >> pal);
										
										if (pri_enable == 0 || gb_game_screen_buffer[pos] == pri_value)
										{
											// pre-palette
											gb_game_screen_buffer[pos] = (val | obj);
										}
									}
								}

								left = (left >> 1);
								right = (right >> 1);
							}

							break;
						}
					}
				}
			}		
		}
	}

	// replace with palette	
	for (int i=0; i<160; i++)
	{
		gb_game_screen_buffer[line + i] = gb_palette_defined[gb_game_screen_buffer[line + i]];
	}
}

// run at 32768 Hz sample rate
void gb_audio()
{
	if (AUDIO_ENABLE == 0) return; // defined above

	if ((gb_aud_nr52 & 0x80) == 0x00) return;

	// channel 1 - pulse (with sweep)
	if ((gb_aud_nr52 & 0x01) == 0x01)
	{
		// period
		gb_ext_ch1_period += 8; // doubled for full wavelength
		
		if (gb_ext_ch1_period >= 2048)
		{
			gb_ext_ch1_period -= 2048;

			gb_ext_ch1_period += (unsigned long)(((gb_aud_nr14 & 0x07) << 8) | (gb_aud_nr13 & 0xFF));
			
			gb_ext_ch1_value = (unsigned short)((gb_ext_ch1_value ^ 0x01FF) & 0x01FF); // max of 0x01FF for each channel
		}

		// length
		if ((gb_aud_nr14 & 0x40) == 0x40)
		{
			gb_ext_ch1_length += 1;

			if (gb_ext_ch1_length >= 8192) // 64 << 7
			{
				gb_aud_nr52 = (unsigned char)(gb_aud_nr52 & 0xFE); // disable channel 1
			}
		}

		// volume/envelope
		if ((gb_aud_nr12 & 0x07) != 0x00)
		{
			gb_ext_ch1_envelope += 1;

			if (gb_ext_ch1_envelope >= ((gb_aud_nr12 & 0x07) << 9))
			{
				gb_ext_ch1_envelope -= ((gb_aud_nr12 & 0x07) << 9);

				if ((gb_aud_nr12 & 0x08) == 0x08)
				{
					if (gb_ext_ch1_volume < 0x0F)
					{
						gb_ext_ch1_volume += 1; // increase volume
					}
				}
				else
				{
					if (gb_ext_ch1_volume > 0x00)
					{
						gb_ext_ch1_volume -= 1; // decrease volume
					}
				}
			}
		}

		// sweep (channel 1 only)
		if ((gb_aud_nr10 & 0x70) != 0x00)
		{
			gb_ext_ch1_sweep += ((gb_aud_nr10 & 0x70) >> 4);
			
			if (gb_ext_ch1_sweep >= 256)
			{
				gb_ext_ch1_sweep -= 256;

				gb_ext_ch1_temp = (unsigned long)(((gb_aud_nr14 & 0x07) << 8) | (gb_aud_nr13 & 0xFF));

				if ((gb_aud_nr10 & 0x08) == 0x08)
				{
					gb_ext_ch1_temp -= (gb_ext_ch1_temp >> (gb_aud_nr10 & 0x07)); // decrease period

					if ((gb_ext_ch1_temp & 0xF800) != 0x0000) // underflow
					{
						gb_ext_ch1_temp = 0x0000;
					}
				}
				else
				{
					gb_ext_ch1_temp += (gb_ext_ch1_temp >> (gb_aud_nr10 & 0x07)); // increase period

					if ((gb_ext_ch1_temp & 0xF800) != 0x0000) // overflow
					{
						gb_ext_ch1_temp = 0x07FF;

						gb_aud_nr52 = (unsigned char)(gb_aud_nr52 & 0xFE); // disable channel 1
					}
				}

				gb_aud_nr13 = (unsigned char)(gb_ext_ch1_temp & 0x00FF);
				gb_aud_nr14 = (unsigned char)((gb_aud_nr14 & 0xF8) | ((gb_ext_ch1_temp & 0x0700) >> 8));
			}
		}
	}
	else
	{
		gb_ext_ch1_value = 0x0000;
	}

	// channel 2 - pulse
	if ((gb_aud_nr52 & 0x02) == 0x02)
	{
		// period
		gb_ext_ch2_period += 8; // doubled for full wavelength
		
		if (gb_ext_ch2_period >= 2048)
		{
			gb_ext_ch2_period -= 2048;

			gb_ext_ch2_period += (unsigned long)(((gb_aud_nr24 & 0x07) << 8) | (gb_aud_nr23 & 0xFF));
			
			gb_ext_ch2_value = (unsigned short)((gb_ext_ch2_value ^ 0x01FF) & 0x01FF); // max of 0x01FF for each channel
		}

		// length
		if ((gb_aud_nr24 & 0x40) == 0x40)
		{
			gb_ext_ch2_length += 1;

			if (gb_ext_ch2_length >= 8192) // 64 << 7
			{
				gb_aud_nr52 = (unsigned char)(gb_aud_nr52 & 0xFD); // disable channel 2
			}
		}

		// volume/envelope
		if ((gb_aud_nr22 & 0x07) != 0x00)
		{
			gb_ext_ch2_envelope += 1;

			if (gb_ext_ch2_envelope >= ((gb_aud_nr22 & 0x07) << 9))
			{
				gb_ext_ch2_envelope -= ((gb_aud_nr22 & 0x07) << 9);

				if ((gb_aud_nr22 & 0x08) == 0x08)
				{
					if (gb_ext_ch2_volume < 0x0F)
					{
						gb_ext_ch2_volume += 1; // increase volume
					}
				}
				else
				{
					if (gb_ext_ch2_volume > 0x00)
					{
						gb_ext_ch2_volume -= 1; // decrease volume
					}
				}
			}
		}
	}
	else
	{
		gb_ext_ch2_value = 0x0000;
	}

	// channel 3 - wave
	if ((gb_aud_nr52 & 0x04) == 0x04 && (gb_aud_nr30 & 0x80) == 0x80)
	{
		// period
		gb_ext_ch3_period += 4; // half freqency of pulse channels
		
		if (gb_ext_ch3_period >= 2048)
		{
			gb_ext_ch3_period -= 2048;

			gb_ext_ch3_period += (unsigned long)(((gb_aud_nr34 & 0x07) << 8) | (gb_aud_nr33 & 0xFF));
			
			if ((gb_ext_ch3_index & 0x01) == 0x00)
			{
				gb_ext_ch3_value = (unsigned short)((gb_mem_wave[(gb_ext_ch3_index >> 1)] & 0xF0) << 1); // max of 0x01FF for each channel
			}
			else
			{
				gb_ext_ch3_value = (unsigned short)((gb_mem_wave[(gb_ext_ch3_index >> 1)] & 0x0F) << 5); // max of 0x01FF for each channel
			}

			gb_ext_ch3_index = (unsigned long)((gb_ext_ch3_index + 1) & 0x1F);
		}

		// length
		if ((gb_aud_nr34 & 0x40) == 0x40)
		{
			gb_ext_ch3_length += 1;

			if (gb_ext_ch3_length >= 32768) // 256 << 7
			{
				gb_aud_nr52 = (unsigned char)(gb_aud_nr52 & 0xFB); // disable channel 3
			}
		}
	}
	else
	{
		gb_ext_ch3_value = 0x0000;
	}

	// channel 4 - noise
	if ((gb_aud_nr52 & 0x08) == 0x08)
	{
		// period
		if ((gb_aud_nr43 & 0xF0) < 0xE0)
		{
			gb_ext_ch4_period += 4; // half the speed of pulse
			
			if (gb_ext_ch4_period >= gb_ext_ch4_divider)
			{
				gb_ext_ch4_period = 0;

				if ((gb_aud_nr43 & 0x07) == 0x00)
				{
					gb_ext_ch4_divider = (unsigned long)((0x01 << ((gb_aud_nr43 & 0xF0) >> 5)));
				}
				else
				{
					gb_ext_ch4_divider = (unsigned long)(((gb_aud_nr43 & 0x07) << ((gb_aud_nr43 & 0xF0) >> 4)));
				}
		
				// using this instead of LFSR
				if (gb_random_table[gb_ext_ch4_random] >= 0x80)
				{
					gb_ext_ch4_value = 0x01FF; // max of 0x01FF for each channel
				}
				else
				{
					gb_ext_ch4_value = 0x0000;
				}

				gb_ext_ch4_random = (unsigned char)(gb_ext_ch4_random + 1);
			}
		}

		// length
		if ((gb_aud_nr44 & 0x40) == 0x40)
		{
			gb_ext_ch4_length += 1;

			if (gb_ext_ch4_length >= 8192) // 64 << 7
			{
				gb_aud_nr52 = (unsigned char)(gb_aud_nr52 & 0xF7); // disable channel 4
			}
		}

		// volume/envelope
		if ((gb_aud_nr42 & 0x07) != 0x00)
		{
			gb_ext_ch4_envelope += 1;

			if (gb_ext_ch4_envelope >= ((gb_aud_nr42 & 0x07) << 9))
			{
				gb_ext_ch4_envelope -= ((gb_aud_nr42 & 0x07) << 9);

				if ((gb_aud_nr42 & 0x08) == 0x08)
				{
					if (gb_ext_ch4_volume < 0x0F)
					{
						gb_ext_ch4_volume += 1; // increase volume
					}
				}
				else
				{
					if (gb_ext_ch4_volume > 0x00)
					{
						gb_ext_ch4_volume -= 1; // decrease volume
					}
				}
			}
		}
	}
	else
	{
		gb_ext_ch4_value = 0x0000;
	}

	// mixer/panning
	gb_game_audio_buffer[gb_game_audio_write] = 0x0000; // unsigned
	gb_game_audio_buffer[gb_game_audio_write] += (unsigned short)(gb_ext_ch1_value * gb_ext_ch1_volume * (((gb_aud_nr51 & 0x10) >> 4) + ((gb_aud_nr51 & 0x01))));
	gb_game_audio_buffer[gb_game_audio_write] += (unsigned short)(gb_ext_ch2_value * gb_ext_ch2_volume * (((gb_aud_nr51 & 0x20) >> 5) + ((gb_aud_nr51 & 0x02) >> 1)));
	gb_game_audio_buffer[gb_game_audio_write] += (unsigned short)(gb_ext_ch3_value * gb_ext_ch3_volume * (((gb_aud_nr51 & 0x40) >> 6) + ((gb_aud_nr51 & 0x04) >> 2)));
	gb_game_audio_buffer[gb_game_audio_write] += (unsigned short)(gb_ext_ch4_value * gb_ext_ch4_volume * (((gb_aud_nr51 & 0x80) >> 7) + ((gb_aud_nr51 & 0x08) >> 3)));

	// master volume
	gb_game_audio_buffer[gb_game_audio_write] = gb_game_audio_buffer[gb_game_audio_write] / (16 - (((gb_aud_nr50 & 0x70) >> 4) + (gb_aud_nr50 & 0x07)));
	
	gb_game_audio_write += 1;

	if (gb_game_audio_write >= AUDIO_LEN)
	{
		gb_game_audio_write -= AUDIO_LEN;
	}
}

// checks for things according to cycles taken
void gb_updates()
{
	// serial cycles
	if ((gb_io_sc & 0x81) == 0x81)
	{
		gb_ext_sb_cycles += gb_cpu_cycles;
		
		if (gb_ext_sb_cycles >= 512)
		{
			gb_ext_sb_cycles = 0;
			
			gb_io_if |= 0x08; // interrupt

			gb_io_sc &= 0x80;
		}
	}

	// lcd cycles
	gb_ext_lx += gb_cpu_cycles;

	if (gb_ext_lx >= 456) // horizontal max
	{
		gb_ext_lx -= 456;
		gb_io_ly += 1;

		if (gb_io_ly == 144) // vblank
		{
			gb_io_if = (gb_io_if | 0x01);
			gb_game_draw = 1;
		}
		else if (gb_io_ly >= 154) // vertical max
		{
			gb_io_ly -= 154;
		}
	}	

	if (gb_io_ly >= 144) // mode 1
	{
		if ((gb_io_stat & 0x03) != 0x01 && (gb_io_stat & 0x10) == 0x10)
		{
			if ((gb_io_lcdc & 0x80) == 0x80) gb_io_if = (gb_io_if | 0x02); // interrupt
		}

		gb_io_stat = ((gb_io_stat & 0xFC) | 0x01);
	}
	else
	{
		if (gb_ext_lx < 80) // mode 2
		{
			if ((gb_io_stat & 0x03) != 0x02 && (gb_io_stat & 0x20) == 0x20)
			{
				if ((gb_io_lcdc & 0x80) == 0x80) gb_io_if = (gb_io_if | 0x02); // interrupt
			}

			gb_io_stat = ((gb_io_stat & 0xFC) | 0x02);
		}
		else if (gb_ext_lx < 252) // mode 3
		{
			gb_io_stat = ((gb_io_stat & 0xFC) | 0x03);
		}
		else // mode 0
		{
			if ((gb_io_stat & 0x03) != 0x00)
			{
				if ((gb_io_stat & 0x08) == 0x08)
				{
					if ((gb_io_lcdc & 0x80) == 0x80) gb_io_if = (gb_io_if | 0x02); // interrupt
				}

				gb_line();
			}

			gb_io_stat = ((gb_io_stat & 0xFC) | 0x00);
		}
	}

	if ((gb_io_lcdc & 0x80) == 0x00) // keep at 0 when off
	{
		gb_ext_lx = 0;
		gb_io_ly = 0;

		gb_io_stat = ((gb_io_stat & 0xFC));
	}

	if (gb_io_ly == gb_io_lyc) // compare
	{
		if ((gb_io_stat & 0x40) == 0x40)
		{
			if ((gb_io_lcdc & 0x80) == 0x80) gb_io_if = (gb_io_if | 0x02); // interrupt
		}

		gb_io_stat = ((gb_io_stat & 0xFB) | 0x04);
	}
	else
	{
		gb_io_stat = ((gb_io_stat & 0xFB) | 0x00);
	}

	// timer cycles
	gb_ext_div_cycles += gb_cpu_cycles;
	gb_ext_tima_cycles += gb_cpu_cycles;

	if (gb_ext_div_cycles >= 256) // div increments
	{	
		gb_ext_div_cycles -= 256;
		gb_io_div = (unsigned char)(gb_io_div + 1);
	}

	if ((gb_io_tac & 0x04) == 0x04) // tima increments
	{
		switch ((gb_io_tac & 0x03))
		{
			case 0x00:
			{
				if (gb_ext_tima_cycles >= 1024)
				{
					gb_ext_tima_cycles -= 1024;
					gb_io_tima = (unsigned char)(gb_io_tima + 1);

					if (gb_io_tima == 0x00) // overflow
					{
						gb_io_tima = (unsigned char)gb_io_tma;

						gb_io_if = (gb_io_if | 0x04);
					}
				}
				break;
			}
			case 0x01:
			{
				if (gb_ext_tima_cycles >= 16)
				{
					gb_ext_tima_cycles -= 16;
					gb_io_tima = (unsigned char)(gb_io_tima + 1);
		
					if (gb_io_tima == 0x00) // overflow
					{
						gb_io_tima = (unsigned char)gb_io_tma;

						gb_io_if = (gb_io_if | 0x04);
					}
				}
				break;
			}
			case 0x02:
			{
				if (gb_ext_tima_cycles >= 64)
				{
					gb_ext_tima_cycles -= 64;
					gb_io_tima = (unsigned char)(gb_io_tima + 1);

					if (gb_io_tima == 0x00) // overflow
					{
						gb_io_tima = (unsigned char)gb_io_tma;

						gb_io_if = (gb_io_if | 0x04);
					}
				}
				break;
			}
			case 0x03:
			{
				if (gb_ext_tima_cycles >= 256)
				{
					gb_ext_tima_cycles -= 256;
					gb_io_tima = (unsigned char)(gb_io_tima + 1);

					if (gb_io_tima == 0x00) // overflow
					{
						gb_io_tima = (unsigned char)gb_io_tma;

						gb_io_if = (gb_io_if | 0x04);
					}
				}
				break;
			}
		}
	}

	// audio cycles
	gb_ext_aud_cycles += gb_cpu_cycles;

	while (gb_ext_aud_cycles >= 128)
	{
		gb_ext_aud_cycles -= 128;

		gb_audio(); // audio at 32768 Hz sample rate
	}

	// joypad
	if ((gb_io_joyp & 0x30) == 0x10) // buttons
	{
		if (((gb_game_buttons_previous ^ gb_game_buttons_current) & 0x0F) != 0x00 &&
			((gb_game_buttons_previous ^ gb_game_buttons_current) & gb_game_buttons_current & 0x0F) == 0x00)
		{
			gb_io_if = (gb_io_if | 0x10); // interrupt
		}
	} 
	else if ((gb_io_joyp & 0x30) == 0x20) // d-pad
	{
		if (((gb_game_buttons_previous ^ gb_game_buttons_current) & 0xF0) != 0x00 &&
			((gb_game_buttons_previous ^ gb_game_buttons_current) & gb_game_buttons_current & 0xF0) == 0x00)
		{
			gb_io_if = (gb_io_if | 0x10); // interrupt
		}
	}

	// clear cycles
	gb_cpu_cycles = 0;
}

// checks for interrupts 
void gb_interrupts()
{
	if ((gb_io_ie & gb_io_if & 0x1F)) // any
	{
		gb_cpu_halt = 0;
	}

	if (gb_cpu_ime == 0x00)
	{
		return;
	}

	if ((gb_io_ie & gb_io_if & 0x01)) // vblank
	{
		gb_io_if = (gb_io_if & 0xFE);
		gb_def_push_16(gb_reg_pc.r16);
		gb_reg_pc.r16 = 0x0040;
		gb_def_cycles(20);
		gb_cpu_ime = 0;
	}
	else if ((gb_io_ie & gb_io_if & 0x02)) // lcd/stat
	{
		gb_io_if = (gb_io_if & 0xFD);
		gb_def_push_16(gb_reg_pc.r16);
		gb_reg_pc.r16 = 0x0048;
		gb_def_cycles(20);
		gb_cpu_ime = 0;
	}
	else if ((gb_io_ie & gb_io_if & 0x04)) // timer
	{
		gb_io_if = (gb_io_if & 0xFB);
		gb_def_push_16(gb_reg_pc.r16);
		gb_reg_pc.r16 = 0x0050;
		gb_def_cycles(20);
		gb_cpu_ime = 0;
	}
	else if ((gb_io_ie & gb_io_if & 0x08)) // serial
	{
		gb_io_if = (gb_io_if & 0xF7);
		gb_def_push_16(gb_reg_pc.r16);
		gb_reg_pc.r16 = 0x0058;
		gb_def_cycles(20);
		gb_cpu_ime = 0;
	}
	else if ((gb_io_ie & gb_io_if & 0x10)) // joypad
	{
		gb_io_if = (gb_io_if & 0xEF);
		gb_def_push_16(gb_reg_pc.r16);
		gb_reg_pc.r16 = 0x0060;
		gb_def_cycles(20);
		gb_cpu_ime = 0;
	}
}


// using OpenAL
unsigned char openal_open()
{
	openal_device = alcOpenDevice(NULL);
	if (!openal_device) return 0;

	openal_context = alcCreateContext(openal_device, NULL);
	if (!openal_context) return 0;

	alcMakeContextCurrent(openal_context);

	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListenerfv(AL_ORIENTATION, listenerOri);

	alGenSources(1, &openal_source);
	if (alGetError() != AL_NO_ERROR) return 0;

	alSourcef(openal_source, AL_PITCH, 1);
	alSourcef(openal_source, AL_GAIN, 1);
	alSource3f(openal_source, AL_POSITION, 0, 0, 0);
	alSource3f(openal_source, AL_VELOCITY, 0, 0, 0);
	alSourcei(openal_source, AL_LOOPING, AL_FALSE);

	return 1;
}

// using OpenAL
void openal_close()
{
	alDeleteSources(1, &openal_source);
    alDeleteBuffers(1, &openal_buffer);
    alcDestroyContext(openal_context);
    alcCloseDevice(openal_device);

	//close(audio_file);
}

// using OpenAL
void openal_play()
{
	if (openal_enable == 0) return;

	for (int i=0; i<AUDIO_LEN; i++) openal_data[i] = (unsigned short)(gb_game_audio_buffer[i]); // unsigned

	alGenBuffers(1, &openal_buffer);
	alBufferData(openal_buffer, AL_FORMAT_MONO16, openal_data, 1098, 32768); // calculation: 70224 / 128 * 2 = 1097.25
	alSourcei(openal_source, AL_BUFFER, openal_buffer);
	alSourcePlay(openal_source);
	
	for (int i=0; i<AUDIO_LEN; i++)
	{
		gb_game_audio_buffer[i] = 0x0000; // unsigned
	}

	gb_game_audio_read = 0;
	gb_game_audio_write = 0;
}

// OpenGL function
void opengl_initialize()
{
	// set up the init settings
	glViewport(0, 0, opengl_size_x, opengl_size_y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glClearColor(0.1f, 0.1f, 0.1f, 0.5f);   

	return;
};

// OpenGL function
void opengl_keys(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		opengl_keyboard_state[key] = 1;

		switch (key)
		{
			case GLFW_KEY_ESCAPE:
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			
				break;
			}
	
			case GLFW_KEY_F1:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);

				opengl_size_x = mode->width;
				opengl_size_y = mode->height;

				break;
			}

			case GLFW_KEY_F2:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				opengl_size_x = SCREEN_X * SCREEN_Z;
				opengl_size_y = SCREEN_Y * SCREEN_Z;

				glfwSetWindowMonitor(window, NULL, 0, 0, opengl_size_x, opengl_size_y, mode->refreshRate);

				break;
			}

			default: {}
		}
	}
	else if (action == GLFW_RELEASE)
	{
		opengl_keyboard_state[key] = 0;
	}

	return;
};

// OpenGL function
void opengl_resize(GLFWwindow *window, int width, int height)
{
	glfwGetWindowSize(window, &width, &height);	

	opengl_size_x = width;
	opengl_size_y = height;
	
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	opengl_initialize();

	return;
};

int main(const int argc, const char **argv)
{
	printf("PICboy\n");
	printf("A fast Gameboy (Color) Emulator designed for Microcontrollers\n");
	printf("Using OpenGL/GLFW for Video and Keyboard, and OpenAL for Audio\n");
	printf("Creator: Professor Steven Chad Burrow, 2026\n");
	printf("Email: stevenchadburrow@gmail.com\n");
	printf("GitHub: github.com/stevenchadburrow\n");
	printf("Public Domain, Mar 2026\n");
	printf("Controls:\n");
	printf("\tWSAD = D-Pad\n");
	printf("\tKJ = AB\n");
	printf("\tBACKSPACE = Select\n");
	printf("\tENTER = Start\n");
	printf("\tIU = Turbo-AB\n");
	printf("\tLO = FastForward/Freeze\n");
	printf("\tB = Save RAM file\n");
	printf("Arguments: <ROM file> [RAM file] [GBC|DMG|RGB|...]\n");

	if (argc < 2)
	{
		printf("Needs ROM file!\n");
	
		return 0;
	}

	unsigned char randomizer = 0;

	for (unsigned long i=0; i<(unsigned long)(time(0) % 1000); i++)
	{
		randomizer = (unsigned char)(rand() % 256);
	}

	FILE *input = NULL;

	input = fopen(argv[1], "rb");
	if (!input)
	{
		printf("Couldn't open ROM file!\n");
		
		return 0;
	}
	
	int bytes = 1;
	unsigned char buffer = 0;
	unsigned long loc = 0;

	while (bytes > 0)
	{
		bytes = fscanf(input, "%c", &buffer);

		if (bytes > 0)	
		{
			gb_mem_rom[loc] = buffer;

			loc++;
		}
	}

	fclose(input);

	printf("ROM Size: %lu\n", loc);

	if (argc >= 3)
	{
		gb_load(argv[2]); // load RAM file
	}

	if (argc >= 4)
	{
		if (argv[3][0] == 'G' &&
			argv[3][1] == 'B' &&
			argv[3][2] == 'C')
		{
			gb_mode = GBC; // Gameboy Color mode
		}
		else if (argv[3][0] == 'D' &&
			argv[3][1] == 'M' &&
			argv[3][2] == 'G')
		{
			gb_mode = DMG; // Original DMG mode
		}
		else
		{
			gb_mode = DMG; // defaulting to Original DMG mode when doing color palettes

			int pal;

			// background/window palette
			pal = 0;
			if (argv[3][0] == 'R') { pal = 4; }
			else if (argv[3][0] == 'G') { pal = 8; }
			else if (argv[3][0] == 'B') { pal = 12; }

			for (int i=0; i<4; i++)
			{
				gb_palette_defined[i] = gb_palette_master[pal+i];
			}

			// object palette 0
			pal = 0;
			if (argv[3][1] == 'R') { pal = 4; }
			else if (argv[3][1] == 'G') { pal = 8; }
			else if (argv[3][1] == 'B') { pal = 12; }

			for (int i=0; i<4; i++)
			{
				gb_palette_defined[i+4] = gb_palette_master[pal+i];
			}

			// object palette 1
			pal = 0;
			if (argv[3][2] == 'R') { pal = 4; }
			else if (argv[3][2] == 'G') { pal = 8; }
			else if (argv[3][2] == 'B') { pal = 12; }

			for (int i=0; i<4; i++)
			{
				gb_palette_defined[i+8] = gb_palette_master[pal+i];
			}
		}
	}		

	gb_initialize();

	if (gb_cart_mbc != 0xFF)
	{
		printf("Found MBC%d Cart ROM\n", (unsigned int)gb_cart_mbc);
	}
	else
	{
		printf("Unsupported Cart ROM\n");

		return 0;
	}

	for (unsigned long i=0; i<SCREEN_X*SCREEN_Y; i++)
	{
		gb_game_screen_buffer[i] = 0;
	}

	openal_open();
	
	// OpenGL initialization
	if (!glfwInit()) return 0;
	opengl_window = glfwCreateWindow(opengl_size_x, opengl_size_y, "PICboy", NULL, NULL);
	if (!opengl_window) { glfwTerminate(); return 0; }
	glfwMakeContextCurrent(opengl_window);
	opengl_initialize();
	for (int i=0; i<512; i++) opengl_keyboard_state[i] = 0;
	glfwSetInputMode(opengl_window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetKeyCallback(opengl_window, opengl_keys);
	glfwSetWindowSizeCallback(opengl_window, opengl_resize);

	while (gb_game_run > 0)
	{ 
		if (glfwWindowShouldClose(opengl_window)) gb_game_run = 0; // makes ESCAPE exit program

		if (gb_game_buttons_freeze_state == 0)
		{
			gb_run();
			gb_updates();
			gb_interrupts();
		}
		else
		{
			gb_game_draw = 1;
		}
		
		if (gb_game_draw > 0)
		{
			gb_game_draw = 0;

			if (gb_game_buttons_fast_forward == 0)
			{
				gb_wait();

				openal_play();
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();

			glBegin(GL_QUADS);

			for (unsigned long j=0; j<SCREEN_Y; j++)
			{
				for (unsigned long i=0; i<SCREEN_X; i++)
				{
					glColor3f((float)((gb_game_screen_buffer[j*SCREEN_X+i] & 0x7C00) >> 10) / 32.0f,
						(float)((gb_game_screen_buffer[j*SCREEN_X+i] & 0x03E0) >> 5) / 32.0f,
						(float)((gb_game_screen_buffer[j*SCREEN_X+i] & 0x001F)) / 32.0f);
					glVertex2f(-1.0f + 1.0f * (float)(i*2+0) / (float)SCREEN_X, 
						1.0f - 1.0f * (float)(j*2+0) / (float)SCREEN_Y);
					glVertex2f(-1.0f + 1.0f * (float)(i*2+0) / (float)SCREEN_X, 
						1.0f - 1.0f * (float)(j*2+2) / (float)SCREEN_Y);
					glVertex2f(-1.0f + 1.0f * (float)(i*2+2) / (float)SCREEN_X, 
						1.0f - 1.0f * (float)(j*2+2) / (float)SCREEN_Y);
					glVertex2f(-1.0f + 1.0f * (float)(i*2+2) / (float)SCREEN_X, 
						1.0f - 1.0f * (float)(j*2+0) / (float)SCREEN_Y);
				}
			}

			glEnd();

			glfwSwapInterval(0); // turn off v-sync
			glfwSwapBuffers(opengl_window);

			glfwPollEvents();

			gb_game_buttons_previous = gb_game_buttons_current; // previous

			if (opengl_keyboard_state[GLFW_KEY_W] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x40); // up
			else gb_game_buttons_current = (gb_game_buttons_current & 0xBF);
			
			if (opengl_keyboard_state[GLFW_KEY_S] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x80); // down
			else gb_game_buttons_current = (gb_game_buttons_current & 0x7F);

			if (opengl_keyboard_state[GLFW_KEY_A] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x20); // left
			else gb_game_buttons_current = (gb_game_buttons_current & 0xDF);
			
			if (opengl_keyboard_state[GLFW_KEY_D] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x10); // right
			else gb_game_buttons_current = (gb_game_buttons_current & 0xEF);

			if (opengl_keyboard_state[GLFW_KEY_K] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x01); // A
			else gb_game_buttons_current = (gb_game_buttons_current & 0xFE);
			
			if (opengl_keyboard_state[GLFW_KEY_J] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x02); // B
			else gb_game_buttons_current = (gb_game_buttons_current & 0xFD);

			if (opengl_keyboard_state[GLFW_KEY_BACKSPACE] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x04); // select
			else gb_game_buttons_current = (gb_game_buttons_current & 0xFB);
			
			if (opengl_keyboard_state[GLFW_KEY_ENTER] == 0) gb_game_buttons_current = (gb_game_buttons_current | 0x08); // start
			else gb_game_buttons_current = (gb_game_buttons_current & 0xF7);

			if (opengl_keyboard_state[GLFW_KEY_L] == 0) gb_game_buttons_fast_forward = 0; // fast-forward
			else gb_game_buttons_fast_forward = 1;

			if (opengl_keyboard_state[GLFW_KEY_O] > 0) // freeze
			{
				if (gb_game_buttons_freeze_hold == 2)
				{
					gb_game_buttons_freeze_hold = 3;
					gb_game_buttons_freeze_state = 1;
				}
				else if (gb_game_buttons_freeze_hold == 0)
				{
					gb_game_buttons_freeze_hold = 1;
					gb_game_buttons_freeze_state = 1;
				}
			}					
			else
			{
				if (gb_game_buttons_freeze_hold == 1)
				{
					gb_game_buttons_freeze_hold = 2;
					gb_game_buttons_freeze_state = 1;
				}
				else if (gb_game_buttons_freeze_hold == 3)
				{
					gb_game_buttons_freeze_hold = 0;
					gb_game_buttons_freeze_state = 0;
				}
			}

			if (opengl_keyboard_state[GLFW_KEY_I] == 0) gb_game_buttons_turbo_a = 0; // turbo A
			else gb_game_buttons_turbo_a = 1;

			if (opengl_keyboard_state[GLFW_KEY_U] == 0) gb_game_buttons_turbo_b = 0; // turbo B
			else gb_game_buttons_turbo_b = 1;

			gb_game_buttons_turbo_timer += 1;

			if (gb_game_buttons_turbo_timer >= 6) // 10 times per seconds
			{
				gb_game_buttons_turbo_timer = 0;

				if (gb_game_buttons_turbo_a > 0) gb_game_buttons_current = (unsigned char)(gb_game_buttons_current ^ 0x01); // toggle A
				if (gb_game_buttons_turbo_b > 0) gb_game_buttons_current = (unsigned char)(gb_game_buttons_current ^ 0x02); // toggle B
			}

			if (opengl_keyboard_state[GLFW_KEY_B] > 0)
			{
				if (gb_game_buttons_saving == 0)
				{
					gb_game_buttons_saving = 1;

					if (argc >= 3)
					{
						gb_save(argv[2]); // from arguments
					}
					else
					{
						gb_save("SaveFile.bin"); // default name
					}
				}
			}
			else
			{
				gb_game_buttons_saving = 0;
			}

			if (gb_game_buttons_saving > 0)
			{
				gb_game_buttons_freeze_state = 1;
			}
		}
	}

	openal_close();

	return 1;
}
