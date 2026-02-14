#include "mintboot/mb_rom.h"

static void mb_kbdvec_stub(void)
{
}

long mb_xbios_kbdvbase(void)
{
	struct mb_kbdvecs {
		uint32_t midivec;
		uint32_t vkbderr;
		uint32_t vmiderr;
		uint32_t statvec;
		uint32_t mousevec;
		uint32_t clockvec;
		uint32_t joyvec;
		uint32_t midisys;
		uint32_t ikbdsys;
		uint8_t ikbdstate;
		uint8_t kbdlength;
		uint16_t pad;
	} __attribute__((packed));
	struct mb_kbdvbase {
		uint32_t kbdvec;
		struct mb_kbdvecs vecs;
	} __attribute__((packed));
	static struct mb_kbdvbase storage;
	static int inited;

	if (!inited) {
		uint32_t stub = (uint32_t)(uintptr_t)mb_kbdvec_stub;
		storage.kbdvec = stub;
		storage.vecs.midivec = stub;
		storage.vecs.vkbderr = stub;
		storage.vecs.vmiderr = stub;
		storage.vecs.statvec = stub;
		storage.vecs.mousevec = stub;
		storage.vecs.clockvec = stub;
		storage.vecs.joyvec = stub;
		storage.vecs.midisys = stub;
		storage.vecs.ikbdsys = stub;
		storage.vecs.ikbdstate = 0;
		storage.vecs.kbdlength = 0;
		inited = 1;
	}

	return (long)(uintptr_t)&storage.vecs;
}
