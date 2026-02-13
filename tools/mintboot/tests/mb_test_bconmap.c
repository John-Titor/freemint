#include "mintboot/mb_portable.h"
#include "mintboot/mb_osbind.h"
#include "mb_tests_internal.h"

struct mb_test_iorec {
	uint8_t *buf;
	uint16_t size;
	volatile uint16_t head;
	volatile uint16_t tail;
	uint16_t low;
	uint16_t high;
} __attribute__((packed));

struct mb_test_ext_iorec {
	struct mb_test_iorec in;
	struct mb_test_iorec out;
	uint8_t baudrate;
	uint8_t flowctrl;
	uint8_t ucr;
	uint8_t datamask;
	uint8_t wr5;
} __attribute__((packed));

struct mb_test_maptab {
	long (*bconstat)(void);
	long (*bconin)(void);
	long (*bcostat)(void);
	long (*bconout)(uint16_t dev, uint16_t c);
	long (*rsconf)(uint16_t baud, uint16_t flow, uint16_t uc, uint16_t rs,
		       uint16_t ts, uint16_t sc);
	struct mb_test_ext_iorec *iorec;
} __attribute__((packed));

struct mb_test_bconmap {
	struct mb_test_maptab *maptab;
	uint16_t maptabsize;
	uint16_t mapped_device;
} __attribute__((packed));

void mb_tests_bconmap(void)
{
	struct mb_test_bconmap *root;
	struct mb_test_ext_iorec *iorec;
	long rc;

	rc = Bconmap(0xffffu);
	if (rc != 6)
		mb_panic("Bconmap current rc=%d expected 6", (int)rc);

	rc = Bconmap(0xfffeu);
	if (rc == 0)
		mb_panic("Bconmap pointer rc=0");

	root = (struct mb_test_bconmap *)(uintptr_t)rc;
	if (root->maptabsize != 1)
		mb_panic("Bconmap maptabsize=%u", (uint32_t)root->maptabsize);
	if (root->mapped_device != 6)
		mb_panic("Bconmap mapped_device=%u", (uint32_t)root->mapped_device);

	if (!root->maptab)
		mb_panic("Bconmap maptab null");
	if (!root->maptab[0].bconstat || !root->maptab[0].bconin ||
	    !root->maptab[0].bcostat || !root->maptab[0].bconout ||
	    !root->maptab[0].rsconf || !root->maptab[0].iorec)
		mb_panic("Bconmap maptab entry missing");

	rc = Bconmap(6);
	if (rc != 6)
		mb_panic("Bconmap set rc=%d expected 6", (int)rc);

	rc = Bconmap(5);
	if (rc != 0)
		mb_panic("Bconmap invalid rc=%d expected 0", (int)rc);

	rc = (long)(uintptr_t)Iorec(0);
	if (rc == -1)
		mb_panic("Iorec(0) returned -1");
	iorec = (struct mb_test_ext_iorec *)(uintptr_t)rc;

	if (!iorec->in.buf || !iorec->out.buf)
		mb_panic("Iorec buffers not set");

	if ((uintptr_t)root->maptab[0].iorec != (uintptr_t)iorec)
		mb_panic("Iorec pointer mismatch");

	rc = (long)(uintptr_t)Iorec(1);
	if (rc != -1)
		mb_panic("Iorec(1) rc=%d expected -1", (int)rc);

	rc = Rsconf(0x1234u, 0x55u, 0x66u, 0xffffu, 0xffffu, 0xffffu);
	if (rc != 0)
		mb_panic("Rsconf rc=%d expected 0", (int)rc);

	if (iorec->baudrate != 0x34u || iorec->flowctrl != 0x55u ||
	    iorec->ucr != 0x66u)
		mb_panic("Rsconf did not update iorec");

	rc = Rsconf(0xffffu, 0xffffu, 0xffffu, 0xffffu, 0xffffu, 0xffffu);
	if (rc != 0)
		mb_panic("Rsconf -1 rc=%d expected 0", (int)rc);
	if (iorec->baudrate != 0x34u || iorec->flowctrl != 0x55u ||
	    iorec->ucr != 0x66u)
		mb_panic("Rsconf -1 updated iorec");
}
