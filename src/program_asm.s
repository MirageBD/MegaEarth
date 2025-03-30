		.rtmodel cpu, "*"
	
		.extern _Zp

 ; -----------------------------------------------------------------------------------------------

		.public program_mainloop
program_mainloop:
		lda 0xd020
		lda 0xd020
		lda 0xd020
		lda 0xd020
		jmp program_mainloop

 ; -----------------------------------------------------------------------------------------------

		.align 256
		.public sine
sine:

		.byte    0,   0,   0,   0,   0,   0,   1,   1,   2,   3,   3,   4,   5,   6,   7,   8
		.byte    9,  10,  12,  13,  15,  16,  18,  19,  21,  23,  25,  27,  29,  31,  33,  35
		.byte   37,  39,  42,  44,  46,  49,  51,  54,  56,  59,  62,  64,  67,  70,  73,  76
		.byte   79,  81,  84,  87,  90,  93,  96,  99, 103, 106, 109, 112, 115, 118, 121, 124
		.byte  128, 131, 134, 137, 140, 143, 146, 149, 152, 156, 159, 162, 165, 168, 171, 174
		.byte  176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216
		.byte  218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 240, 242, 243, 245
		.byte  246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255, 255, 255, 255

/*
		.byte   12,  11,   9,   7,   6,   5,   4,   3,   2,   1,   1,   0,   0,   0,   0,   0
		.byte    0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   9,  10,  12,  14,  16,  18
		.byte   20,  22,  24,  27,  29,  32,  35,  38,  41,  44,  47,  50,  53,  57,  60,  64
		.byte   67,  71,  75,  78,  82,  86,  90,  94,  98, 102, 106, 110, 114, 118, 122, 126
		.byte  130, 134, 138, 142, 146, 151, 155, 158, 162, 166, 170, 174, 178, 182, 185, 189
		.byte  192, 196, 199, 203, 206, 209, 212, 215, 218, 221, 224, 226, 229, 231, 234, 236
		.byte  238, 240, 242, 244, 245, 247, 248, 250, 251, 252, 253, 253, 254, 255, 255, 255
		.byte  255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 248, 247, 246, 244, 242
*/		