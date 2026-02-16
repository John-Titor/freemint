#include "mintboot/mb_board.h"
#include "mintboot/mb_common.h"
#include "mintboot/mb_rom.h"
#include "mintboot/mb_lowmem.h"
#include "mintboot/mb_linea.h"
#include "mintboot/mb_trap_helpers.h"
#include "mintboot/mb_cpu.h"
#include "mintboot/mb_rom.h"

#include <stdint.h>
#include "mintboot/mb_lib.h"

#pragma pack(push, 1)
typedef struct
{
    uint32_t d[8]; // added by mb_trap_fatal
    uint32_t a[8]; // added by mb_trap_fatal
    uint16_t sr;
    uint32_t pc;
    uint16_t format:4;
    uint16_t vector:10;
    uint16_t       :2;
    union {
    	struct {
    		uint32_t	address;
    	} format_0x2;
    	struct {
    		uint32_t	effective_address;
    	} format_0x3;
    	struct {
    		uint32_t	effective_address;
    		uint32_t	faulting_pc;
    	} format_0x4;
    	struct {
    		uint32_t effective_address;
    		uint16_t ssw_cp  :1;
    		uint16_t ssw_cu  :1;
    		uint16_t ssw_ct  :1;
    		uint16_t ssw_cm  :1;
    		uint16_t ssw_ma  :1;
    		uint16_t ssw_atc :1;
    		uint16_t ssw_lk  :1;
    		uint16_t ssw_rw  :1;
    		uint16_t ssw_x   :1;
    		uint16_t ssw_size:2;
    		uint16_t ssw_tt  :2;
    		uint16_t ssw_tm  :3;
    		uint16_t writeback_3_status;
    		uint16_t writeback_2_status;
    		uint16_t writeback_1_status;
    		uint32_t fault_address;
    		uint32_t writeback_3_address;
    		uint32_t writeback_3_data;
    		uint32_t writeback_2_address;
    		uint32_t writeback_2_data;
    		uint32_t writeback_1_address;
    		uint32_t writeback_1_data;
    		uint32_t push_data_1;
    		uint32_t push_data_2;
    		uint32_t push_data_3;
    	} format_0x7;
    };
} exception_frame;
#pragma pack(pop)

MB_COV_EXPECT_UNHIT void mb_trap_panic(exception_frame frame);

extern void mb_trap_fatal(void);
extern void mb_line1010(void);
extern void mb_trap1_stub(void);
extern void mb_trap1(void);
extern void mb_trap2(void);
extern void mb_trap13(void);
extern void mb_trap14(void);

void mb_common_setup_vectors(void)
{
	volatile void **vectors = (volatile void **)mb_common_vector_base();
	uint32_t i;

	/* build the default table */
	for (i = 2; i < 256; i++) {
		void *vec = mb_trap_fatal;
		switch (i) {
		case 10: vec = (void *)mb_line1010; break;
		//case 33: vec = (void *)mb_trap1_stub; break;
		case 33: vec = (void *)mb_trap1; break;
		case 34: vec = (void *)mb_trap2; break;
		case 45: vec = (void *)mb_trap13; break;
		case 46: vec = (void *)mb_trap14; break;
		}
		vectors[i] = vec;
	}
}

static const char *const exception_names[] = {
    "impossible",                              //  0
    "impossible",                              //  1
    "access error",                            //  2
    "address error",                           //  3
    "illegal instruction",                     //  4
    "zero divide",                             //  5
    "chk",                                     //  6
    "trapv",                                   //  7
    "privilege violation",                     //  8
    "trace",                                   //  9
    "line a",                                  // 10
    "line f",                                  // 11
    "emulator interrupt",                      // 12
    "coprocessor protocol violation",          // 13
    "format error",                            // 14
    "uninitialized interrupt",                 // 15
    "reserved",                                // 16
    "reserved",                                // 17
    "reserved",                                // 18
    "reserved",                                // 19
    "reserved",                                // 20
    "reserved",                                // 21
    "reserved",                                // 22
    "reserved",                                // 23
    "spurious interrupt",                      // 24
    "level 1 autovector",                      // 25
    "level 2 autovector",                      // 26
    "level 3 autovector",                      // 27
    "level 4 autovector",                      // 28
    "level 5 autovector",                      // 29
    "level 6 autovector",                      // 30
    "level 7 autovector",                      // 31
    "trap 0",                                  // 32
    "trap 1",                                  // 33
    "trap 2",                                  // 34
    "trap 3",                                  // 35
    "trap 4",                                  // 36
    "trap 5",                                  // 37
    "trap 6",                                  // 38
    "trap 7",                                  // 39
    "trap 8",                                  // 40
    "trap 9",                                  // 41
    "trap 10",                                 // 42
    "trap 11",                                 // 43
    "trap 12",                                 // 44
    "trap 13",                                 // 45
    "trap 14",                                 // 46
    "trap 15",                                 // 47
    "FP branch or set on unordered condition", // 48
    "FP inexact result",                       // 49
    "FP divide by zero",                       // 50
    "FP underflow",                            // 51
    "FP operand error",                        // 52
    "FP overflow",                             // 53
    "FP signaling NAN",                        // 54
    "FP unimplemented data type",              // 55
    "MMU configuration error",                 // 56
    "reserved",                                // 57
    "reserved",                                // 58
    "reserved",                                // 59
    "unimplemented effective address",         // 60
    "unimplemented integer instruction",       // 61
    "reserved",                                // 62
    "reserved",                                // 63
};

static const char * const transfer_modes[] = {"data cache push",
                                       "user data",
                                       "user code",
                                       "table search code",
                                       "table search data",
                                       "supervisor data",
                                       "supervisor code",
                                       "reserved"};

MB_COV_EXPECT_UNHIT
static const char *
exception_name(const exception_frame *frame)
{
    return (frame->vector < 64) ? exception_names[frame->vector] : "interrupt";
}

MB_COV_EXPECT_UNHIT NORETURN
void mb_trap_panic(exception_frame _frame)
{
	exception_frame *frame = &_frame;

    mb_log_printf("mintboot: %s [%d] ", exception_name(frame), frame->vector);

    switch (frame->format) {
    case 0x2: {
        mb_log_printf("address 0x%08x\n", frame->format_0x2.address);
    } break;
    case 0x3: {
        mb_log_printf("effective address 0x%08x\n", frame->format_0x3.effective_address);
    } break;
    case 0x4: {
        mb_log_printf("effective address 0x%08x  faulting pc 0x%08x\n",
                      frame->format_0x4.effective_address,
                      frame->format_0x4.faulting_pc);
    } break;
    case 0x7: {
        mb_log_printf("effective address 0x%08x  fault address 0x%08x\n",
                      frame->format_0x7.effective_address,
                      frame->format_0x7.fault_address);
        if (frame->format_0x7.ssw_atc) {
            mb_log_printf("  translation error on %s %s, SIZE=%d TT=%d\n",
                         transfer_modes[frame->format_0x7.ssw_tm],
                         frame->format_0x7.ssw_rw ? "read" : "write",
                         frame->format_0x7.ssw_size,
                         frame->format_0x7.ssw_tt);
        }
    } break;
    default:
        // no additional info
        mb_log_printf(" (frame format %d)\n", frame->format);
    }

    mb_log_printf("PC  0x%08x     SR  0x%04x\n", frame->pc, frame->sr);
    mb_log_printf("USP 0x%08x%s  SSP 0x%08x%s\n",
                mb_cpu_get_usp(),
                ((frame->sr & 0x3000) == 0x0000) ? "<<<" : "   ",
                mb_cpu_get_ssp(),
                ((frame->sr & 0x3000) == 0x2000) ? "<<<" : "   ");
//    mb_log_printf("URP 0x%08x     SRP 0x%08x     TC  0x%08x\n", get_urp(), get_srp(), get_tc());
    for (unsigned i = 0U; i < 8U; i++) {
        mb_log_printf(" D%d 0x%08x %u\n",
                    i,
                    frame->d[i],
                    frame->d[i]);
    }
    for (unsigned i = 0U; i < 8U; i++) {
        mb_log_printf(" A%d 0x%08x %u\n",
                    i,
                    frame->a[i],
                    frame->a[i]);
    }
    mb_panic("mintboot: fatal exception\n");
}
