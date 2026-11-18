#!/usr/bin/env python3
"""Split the inherited SWEET/Lavender early console into board-specific units.

This helper runs from the pinned darwin-xnu-build checkout (dxb), so the
patched XNU source tree is relative to the current directory as xnu/.

The pinned XNU patch stack adds Lavender bring-up to sweet_early.c for historical
compatibility. Run this after the complete patch stack and before compilation.
"""
from pathlib import Path

ROOT = Path("xnu")
SOURCE = ROOT / "osfmk/arm64/sweet_early.c"
SWEET_HEADER = ROOT / "osfmk/arm64/sweet_early.h"
LAVENDER_SOURCE = ROOT / "osfmk/arm64/lavender_early.c"
LAVENDER_HEADER = ROOT / "osfmk/arm64/lavender_early.h"
FILES = ROOT / "osfmk/conf/files.arm64"

COMPAT_WRAPPER = (
    "\n/* Linker compatibility wrapper for translation units that declare\n"
    " * extern void sweet_early_trace_startup(...) without sweet_early.h. */\n"
    "#undef sweet_early_trace_startup\n"
    "void sweet_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,\n"
    "    uintptr_t c, uintptr_t d, uintptr_t e);\n"
    "void\n"
    "sweet_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,\n"
    "    uintptr_t c, uintptr_t d, uintptr_t e)\n"
    "{\n"
    "\tlavender_early_trace_startup(event, a, b, c, d, e);\n"
    "}\n"
)

def ensure_guardian_late_progress():
    lavender = LAVENDER_SOURCE.read_text()
    declaration = "static volatile uint32_t *early_guard_cell;"
    if "lavender_guard_stage20_complete" not in lavender:
        if lavender.count(declaration) != 1:
            raise SystemExit("could not locate unique Lavender Guardian marker declaration")
        lavender = lavender.replace(
            declaration,
            declaration + "\nstatic unsigned int lavender_guard_stage20_complete;",
            1
        )

    stage_anchor = r'''\t\t*early_guard_cell = (stage == 20) ? (0x80000100u | stage) : (0x100u | stage);
\t\t__asm__ volatile("dc cvac, %0\n\tdsb sy" :: "r"(early_guard_cell) : "memory");
'''
    stage_insert = stage_anchor + "\t\tif (stage == 20) lavender_guard_stage20_complete = 1;\n"
    if "if (stage == 20) lavender_guard_stage20_complete = 1;" not in lavender:
        if lavender.count(stage_anchor) != 1:
            raise SystemExit("could not locate stage-20 Guardian marker publication")
        lavender = lavender.replace(stage_anchor, stage_insert, 1)

    trace_anchor = """lavender_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,
    uintptr_t c, uintptr_t d, uintptr_t e)
{
\tchar text[128];
"""
    trace_insert = """lavender_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,
    uintptr_t c, uintptr_t d, uintptr_t e)
{
\t/* Retain stage-20's high-bit handoff flag while publishing later XNU progress. */
\tif (early_guard_cell != 0 && lavender_guard_stage20_complete) {
\t\t*early_guard_cell = 0x80010000u | (event & 0xffffu);
\t\t__asm__ volatile("dc cvac, %0\\n\\tdsb sy" :: "r"(early_guard_cell) : "memory");
\t}
\tchar text[128];
"""
    if "0x80010000u | (event & 0xffffu)" not in lavender:
        if lavender.count(trace_anchor) != 1:
            raise SystemExit("could not locate unique Lavender startup trace entry")
        lavender = lavender.replace(trace_anchor, trace_insert, 1)

    old_comment = """ * Keep a single rolling status line below the ordinary checkpoint history.
 * Do not publish these sub-events to the Guardian cell: they must not
 * extend its 60-second no-progress deadline or collide with the stage20 high bit.
"""
    new_comment = """ * Keep a single rolling status line below the ordinary checkpoint history.
 * After stage 20, publish each later startup event as 0x80010000|event in
 * the Guardian cell, preserving the high-bit handoff-complete flag.
"""
    if old_comment in lavender:
        lavender = lavender.replace(old_comment, new_comment, 1)
    LAVENDER_SOURCE.write_text(lavender)


def ensure_post_platform_renderer():
    lavender = LAVENDER_SOURCE.read_text()
    # IOKit bring-up checkpoints are injected after the historical combined
    # source has been split. Patch 0126 runs before this file exists.
    trace_anchor = '\tcase 46: lavender_trace_append_text(text, &length, "KBOOT KERNEL_THREAD_CREATE DONE"); break;'
    trace_labels = (
        '\n\tcase 107: lavender_trace_append_text(text, &length, "PE_INIT_IOKIT ENTER"); break;'
        '\n\tcase 108: lavender_trace_append_text(text, &length, "PE_INIT_PRINTF DONE"); break;'
        '\n\tcase 109: lavender_trace_append_text(text, &length, "IBOOT VERSION PRINT DONE"); break;'
        '\n\tcase 110: lavender_trace_append_text(text, &length, "PE MEMORY MAP CHECK DONE"); break;'
        '\n\tcase 111: lavender_trace_append_text(text, &length, "PE PREPARE IMAGES DONE"); break;'
        '\n\tcase 112: lavender_trace_append_text(text, &length, "PE PROGRESS SETUP DONE"); break;'
        '\n\tcase 113: lavender_trace_append_text(text, &length, "PE IBOOT TIMING CHECK DONE"); break;'
        '\n\tcase 114: lavender_trace_append_text(text, &length, "IOKIT INIT CALL BEGIN"); break;'
        '\n\tcase 115: lavender_trace_append_text(text, &length, "IOKIT INIT ENTER"); break;'
        '\n\tcase 116: lavender_trace_append_text(text, &length, "IOKIT IOLIB DONE"); break;'
        '\n\tcase 117: lavender_trace_append_text(text, &length, "IOKIT OSLIBKERN DONE"); break;'
        '\n\tcase 118: lavender_trace_append_text(text, &length, "IOKIT MACH PORTS DONE"); break;'
        '\n\tcase 119: lavender_trace_append_text(text, &length, "IOKIT GLOBALS DONE"); break;'
        '\n\tcase 120: lavender_trace_append_text(text, &length, "IOKIT ROOT NUB ALLOCATED"); break;'
        '\n\tcase 121: lavender_trace_append_text(text, &length, "IOKIT ROOT NUB INIT DONE"); break;'
        '\n\tcase 122: lavender_trace_append_text(text, &length, "IOKIT ROOT NUB ATTACH DONE"); break;'
        '\n\tcase 123: lavender_trace_append_text(text, &length, "IOKIT INIT DONE"); break;'
        '\n\tcase 124: lavender_trace_append_text(text, &length, "IOKIT CONFIGURE BEGIN"); break;'
        '\n\tcase 125: lavender_trace_append_text(text, &length, "IOKIT CONFIGURE RETURNED"); break;'
    )
    root_init_trace_labels = (
        '\n\tcase 126: lavender_trace_append_text(text, &length, "IOKIT ROOT NUB INIT ENTER dtRoot="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 127: lavender_trace_append_text(text, &length, "IOKIT DEVICE TREE ALLOC BEGIN dtRoot="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 128: lavender_trace_append_text(text, &length, "IOKIT DEVICE TREE ALLOC RETURNED dt="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 129: lavender_trace_append_text(text, &length, "IOKIT ROOT REGISTRY INIT BEGIN dt="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 130: lavender_trace_append_text(text, &length, "IOKIT ROOT REGISTRY INIT RETURNED ok="); lavender_trace_append_decimal(text, &length, a); break;'
        '\n\tcase 131: lavender_trace_append_text(text, &length, "IOKIT ROOT REGISTRY FALLBACK BEGIN"); break;'
        '\n\tcase 132: lavender_trace_append_text(text, &length, "IOKIT ROOT REGISTRY FALLBACK RETURNED ok="); lavender_trace_append_decimal(text, &length, a); break;'
    )

    allocator_trace_labels = (
        '\n\tcase 133: lavender_trace_append_text(text, &length, "DT ALLOC ENTER dt="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 134: lavender_trace_append_text(text, &length, "DT PLANE READY ptr="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 135: lavender_trace_append_text(text, &length, "DT SYMBOLS READY name="); lavender_trace_append_hex(text, &length, a); lavender_trace_append_text(text, &length, " persist="); lavender_trace_append_hex(text, &length, b); break;'
        '\n\tcase 136: lavender_trace_append_text(text, &length, "DT GLOBALS READY resolvers="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 137: lavender_trace_append_text(text, &length, "DT ROOT BUILD BEGIN dt="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 138: lavender_trace_append_text(text, &length, "DT ROOT BUILD DONE entry="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 139: lavender_trace_append_text(text, &length, "DT STACK READY ptr="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 140: lavender_trace_append_text(text, &length, "DT ITERATOR READY root="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 141: lavender_trace_append_text(text, &length, "DT CHILD BUILD n="); lavender_trace_append_decimal(text, &length, a); lavender_trace_append_text(text, &length, " ptr="); lavender_trace_append_hex(text, &length, b); break;'
        '\n\tcase 142: lavender_trace_append_text(text, &length, "DT WALK DONE nodes="); lavender_trace_append_decimal(text, &length, a); break;'
        '\n\tcase 143: lavender_trace_append_text(text, &length, "DT ROOT ATTACHED ptr="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 144: lavender_trace_append_text(text, &length, "DT INTERRUPT WALK READY iter="); lavender_trace_append_hex(text, &length, a); break;'
        '\n\tcase 145: lavender_trace_append_text(text, &length, "DT INTERRUPT WALK DONE map="); lavender_trace_append_decimal(text, &length, a); break;'
        '\n\tcase 146: lavender_trace_append_text(text, &length, "DT ALLOC RETURN root="); lavender_trace_append_hex(text, &length, a); break;'
    )
    if "case 146: lavender_trace_append_text" not in lavender:
        allocator_anchor = 'case 132: lavender_trace_append_text(text, &length, "IOKIT ROOT REGISTRY FALLBACK RETURNED ok="); lavender_trace_append_decimal(text, &length, a); break;'
        if allocator_anchor not in lavender:
            raise SystemExit("could not find event 132 anchor for DeviceTree allocator labels")
        lavender = lavender.replace(allocator_anchor, allocator_anchor + allocator_trace_labels, 1)
        LAVENDER_SOURCE.write_text(lavender)
    ensure_guardian_late_progress()
    if "case 125: lavender_trace_append_text" not in lavender:
        if trace_anchor not in lavender:
            raise SystemExit("could not find event 46 anchor for IOKit trace labels")
        lavender = lavender.replace(trace_anchor, trace_anchor + trace_labels, 1)
        LAVENDER_SOURCE.write_text(lavender)

    if "case 132: lavender_trace_append_text" not in lavender:
        if "case 126: lavender_trace_append_text" in lavender:
            raise SystemExit("partial root-nub trace labels found; refusing to duplicate them")
        root_anchor = '\tcase 125: lavender_trace_append_text(text, &length, "IOKIT CONFIGURE RETURNED"); break;'
        if root_anchor not in lavender:
            raise SystemExit("could not find event 125 anchor for root-nub trace labels")
        lavender = lavender.replace(root_anchor, root_anchor + root_init_trace_labels, 1)
        LAVENDER_SOURCE.write_text(lavender)
    old = ('default:\n\t\tlavender_trace_append_text(text, &length, "UNKNOWN STARTUP TRACE");\n'
           '\t\tbreak;')
    new = ('default:\n'
           '\t\tif (event >= 47 && event <= 106) {\n'
           '\t\t\tlavender_trace_append_text(text, &length, "POST-PLATFORM EVENT ");\n'
           '\t\t\tlavender_trace_append_decimal(text, &length, event);\n'
           '\t\t\tif (a != 0 || b != 0) {\n'
           '\t\t\t\tlavender_trace_append_text(text, &length, " a=");\n'
           '\t\t\t\tlavender_trace_append_hex(text, &length, a);\n'
           '\t\t\t\tlavender_trace_append_text(text, &length, " b=");\n'
           '\t\t\t\tlavender_trace_append_hex(text, &length, b);\n'
           '\t\t\t}\n'
           '\t\t} else {\n'
           '\t\t\tlavender_trace_append_text(text, &length, "UNKNOWN STARTUP TRACE");\n'
           '\t\t}\n'
           '\t\tbreak;')
    if new in lavender:
        return
    if old not in lavender:
        raise SystemExit("could not find expected Lavender trace renderer to upgrade")
    LAVENDER_SOURCE.write_text(lavender.replace(old, new, 1))


def ensure_trace_compatibility():
    lavender = LAVENDER_SOURCE.read_text()
    if "void sweet_early_trace_startup(" not in lavender:
        LAVENDER_SOURCE.write_text(lavender + COMPAT_WRAPPER)


def main():
    # Newer Lavender patch stacks may already provide the board-specific file
    # and remove the historical combined SWEET source before this helper runs.
    # Treat that state as complete only when every generated interface and
    # the ARM64 source-list entry are present.
    if not SOURCE.exists():
        required = (LAVENDER_SOURCE, LAVENDER_HEADER, SWEET_HEADER, FILES)
        if all(path.exists() for path in required):
            lavender = LAVENDER_SOURCE.read_text()
            files = FILES.read_text()
            if ("lavender_early_trace_startup" in lavender and
                    "osfmk/arm64/lavender_early.c\tstandard" in files):
                ensure_post_platform_renderer()
                ensure_trace_compatibility()
                print("Lavender early source is already split; historical SWEET source is absent")
                return
        raise SystemExit("sweet_early.c is absent and the Lavender split is incomplete")

    source = SOURCE.read_text()
    marker = "#elif defined(ARM64_BOARD_CONFIG_LAVENDER)"
    end_marker = "#endif /* SWEET || ARM64_BOARD_CONFIG_LAVENDER */"
    if marker not in source:
        if LAVENDER_SOURCE.exists() and "lavender_early_trace_startup" in LAVENDER_SOURCE.read_text():
            ensure_post_platform_renderer()
            ensure_trace_compatibility()
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
                        "#include <arm64/sweet_early.h>\n\n/* LAVENDER_XNU_GUARD_2026:", 1)
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
        + COMPAT_WRAPPER
    )
    ensure_post_platform_renderer()
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
/* Keep the exported compatibility header self-contained: custom board headers
 * are not part of XNU's generated EXPORT_HDRS set. */
void lavender_early_mark(unsigned int stage, const char *label);
void lavender_early_putc(char c);
void lavender_early_panic(const char *fmt, va_list *args);
void lavender_early_trace_startup(unsigned int event, uintptr_t a, uintptr_t b,
    uintptr_t c, uintptr_t d, uintptr_t e);
void lavender_early_relocate(uint64_t base);
void lavender_early_disable(void);
void lavender_early_guard_init(uintptr_t boot_args_kva);
void lavender_early_logo(void);
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
