#!/usr/bin/env python3
"""Split the inherited SWEET/Lavender early console into board-specific units.

The pinned XNU patch stack adds Lavender bring-up to sweet_early.c for historical
compatibility. Run this after the complete patch stack and before compilation.
"""
from pathlib import Path

ROOT = Path("dxb/xnu")
SOURCE = ROOT / "osfmk/arm64/sweet_early.c"
SWEET_HEADER = ROOT / "osfmk/arm64/sweet_early.h"
LAVENDER_SOURCE = ROOT / "osfmk/arm64/lavender_early.c"
LAVENDER_HEADER = ROOT / "osfmk/arm64/lavender_early.h"
FILES = ROOT / "osfmk/conf/files.arm64"

def main():
    source = SOURCE.read_text()
    marker = "#elif defined(ARM64_BOARD_CONFIG_LAVENDER)"
    end_marker = "#endif /* SWEET || ARM64_BOARD_CONFIG_LAVENDER */"
    if marker not in source:
        if LAVENDER_SOURCE.exists() and "lavender_early_trace_startup" in LAVENDER_SOURCE.read_text():
            print("Lavender early source is already split")
            return
        raise SystemExit("combined sweet_early.c is missing its Lavender branch")
    if source.count(marker) != 1 or source.count(end_marker) != 1:
        raise SystemExit("unexpected SWEET/Lavender source boundaries")

    start = source.index(marker)
    end = source.rindex(end_marker)
    body = source[start + len(marker):end].strip() + "\n"
    body = body.replace("#include <stdint.h>\n",
                        "#include <stdint.h>\n#include <kern/misc_protos.h>\n", 1)
    body = body.replace("/* LAVENDER_XNU_GUARD_2026:",
                        "#include <arm64/lavender_early.h>\n\n/* LAVENDER_XNU_GUARD_2026:", 1)
    for old, new in (
        ("sweet_early_trace_startup", "lavender_early_trace_startup"),
        ("sweet_early_guard_init", "lavender_early_guard_init"),
        ("sweet_early_logo", "lavender_early_logo"),
        ("sweet_early_mark", "lavender_early_mark"),
        ("sweet_early_putc", "lavender_early_putc"),
        ("sweet_early_panic", "lavender_early_panic"),
        ("sweet_early_relocate", "lavender_early_relocate"),
        ("sweet_early_disable", "lavender_early_disable"),
    ):
        body = body.replace(old, new)

    LAVENDER_SOURCE.write_text(
        "/*\n * Lavender early boot console and diagnostics.\n"
        " * Separate from the SWEET/SM7150 implementation.\n"
        " * Uses the verified Lavender boot framebuffer and kernel alias.\n */\n"
        + body
    )
    SOURCE.write_text(source[:start] + "#endif /* SWEET */\n")

    LAVENDER_HEADER.write_text("""/* Lavender-only early framebuffer console and boot diagnostics. */
#ifndef _ARM64_LAVENDER_EARLY_H_
#define _ARM64_LAVENDER_EARLY_H_

#include <stdarg.h>
#include <stdint.h>

#if defined(ARM64_BOARD_CONFIG_LAVENDER)
void lavender_early_mark(unsigned int stage, const char *label);
void lavender_early_putc(char c);
void lavender_early_panic(const char *fmt, va_list *args);
void lavender_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,
    uintptr_t c, uintptr_t d, uintptr_t e);
void lavender_early_relocate(uint64_t base);
void lavender_early_disable(void);
void lavender_early_guard_init(uintptr_t boot_args_kva);
void lavender_early_logo(void);
#else
static inline void lavender_early_mark(unsigned int stage, const char *label)
{ (void)stage; (void)label; }
static inline void lavender_early_disable(void) {}
static inline void lavender_early_guard_init(uintptr_t boot_args_kva)
{ (void)boot_args_kva; }
#endif

#endif /* _ARM64_LAVENDER_EARLY_H_ */
""")

    SWEET_HEADER.write_text("""/* Compatibility facade for shared XNU call sites. */
#ifndef _ARM64_SWEET_EARLY_H_
#define _ARM64_SWEET_EARLY_H_

#include <pexpert/arm64/board_config.h>
#include <stdarg.h>
#include <stdint.h>

#if defined(ARM64_BOARD_CONFIG_LAVENDER)
#include <arm64/lavender_early.h>
#define sweet_early_mark lavender_early_mark
#define sweet_early_putc lavender_early_putc
#define sweet_early_panic lavender_early_panic
#define sweet_early_trace_startup lavender_early_trace_startup
#define sweet_early_relocate lavender_early_relocate
#define sweet_early_disable lavender_early_disable
#define sweet_early_guard_init lavender_early_guard_init
#define sweet_early_logo lavender_early_logo
#elif defined(SWEET)
void sweet_early_mark(unsigned int stage, const char *label);
void sweet_early_putc(char c);
void sweet_early_panic(const char *fmt, va_list *args);
void sweet_early_relocate(uint64_t base);
void sweet_early_disable(void);
static inline void sweet_early_guard_init(uintptr_t boot_args_kva)
{ (void)boot_args_kva; }
#else
static inline void sweet_early_mark(unsigned int stage, const char *label)
{ (void)stage; (void)label; }
static inline void sweet_early_disable(void) {}
static inline void sweet_early_guard_init(uintptr_t boot_args_kva)
{ (void)boot_args_kva; }
#endif

#endif /* _ARM64_SWEET_EARLY_H_ */
""")

    files = FILES.read_text()
    entry = "osfmk/arm64/lavender_early.c\tstandard"
    if entry not in files:
        sweet_entry = "osfmk/arm64/sweet_early.c\tstandard"
        if files.count(sweet_entry) != 1:
            raise SystemExit("could not locate unique sweet_early.c source-list entry")
        FILES.write_text(files.replace(sweet_entry, sweet_entry + "\n" + entry, 1))

    if "ARM64_BOARD_CONFIG_LAVENDER" in SOURCE.read_text():
        raise SystemExit("SWEET source still contains a Lavender implementation")
    if "lavender_early_trace_startup" not in LAVENDER_SOURCE.read_text():
        raise SystemExit("generated Lavender source is missing startup tracing")
    print("PASS: Lavender early console split into lavender_early.c/.h")

if __name__ == "__main__":
    main()
