#pragma once
#include <stdint.h>

/* Redmi Note 7 / SDM660 — values cross-checked against the embedded FDT in
 * LucidOS Build 4 and the supplied live hardware report. */
#define FB_BASE         0x9d400000UL
#define FB_PHYS         FB_BASE
#define FB_SIZE         0x23ff000UL
#define FB_W            1080
#define FB_H            2340
#define FB_STRIDE       (FB_W * 4)

#define GICD_BASE       0x17a00000UL
#define GICR_BASE       0x17b00000UL
#define GICR_SIZE       0x100000UL
#define GICR_PE_SIZE    0x20000UL
#define VTIMER_INTID    19
#define SGI_INTID       1
#define NR_CPUS         8

#define I2C0_BASE       0x0c175000UL
#define I2C0_SIZE       0x600UL
#define TLMM_BASE       0x03000000UL
#define NVT_I2C_ADDR    0x62
#define NVT_IRQ_GPIO    67
#define NVT_RESET_GPIO  66
#define EMMC_CQHCI_BASE 0x0c0c5000UL
#define EMMC_ICE_BASE   0x0c0c8000UL
#define LAVENDER_CPU_MPIDRS { 0x000, 0x001, 0x002, 0x003, 0x100, 0x101, 0x102, 0x103 }

