#!/usr/bin/env python3
"""Install deterministic RPM object/symbol checks at XNU's final-link recipes."""
from pathlib import Path
import sys

NON_LTO_OLD = (
    '\t$(_v)$(CAT) $(filter %.filelist,$+) < /dev/null > link.filelist\n'
    '\t$(_v)$(LD) $(LDFLAGS_KERNEL) $(LDFLAGS_KERNEL_ONLY) -filelist link.filelist version.o $(filter %.o,$+) -o $@ $(LD_KERNEL_LIBS) $(LD_KERNEL_ARCHIVES)\n'
)
NON_LTO_NEW = (
    '\t$(_v)$(CAT) $(filter %.filelist,$+) < /dev/null > link.filelist\n'
    '\t$(_v)python3 "$(SRCROOT)/../../tools/verify-lavender-rpm-link-input.py" --non-lto link.filelist\n'
    '\t$(_v)$(LD) $(LDFLAGS_KERNEL) $(LDFLAGS_KERNEL_ONLY) -filelist link.filelist version.o $(filter %.o,$+) -o $@ $(LD_KERNEL_LIBS) $(LD_KERNEL_ARCHIVES)\n'
)
PRE_LTO_OLD = (
    "\tprintf '%s\\n' \"$$nonlto\" >nonltolink.filelist\n"
    "\t@$(LOG_LD) \"$(@F)\"\n"
)
PRE_LTO_NEW = (
    "\tprintf '%s\\n' \"$$nonlto\" >nonltolink.filelist\n"
    "\t$(_v)python3 \"$(SRCROOT)/../../tools/verify-lavender-rpm-link-input.py\" --pre-lto ltolink.filelist nonltolink.filelist\n"
    "\t@$(LOG_LD) \"$(@F)\"\n"
)


def replace_once(source: str, old: str, new: str, label: str) -> str:
    count = source.count(old)
    if count != 1:
        raise SystemExit(f"expected exactly one {label} anchor in MakeInc.kernel, found {count}")
    return source.replace(old, new, 1)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: inject-lavender-rpm-link-guard.py PATH_TO_MakeInc.kernel")
    path = Path(sys.argv[1])
    source = path.read_text()
    non_lto_marker = 'verify-lavender-rpm-link-input.py" --non-lto link.filelist'
    pre_lto_marker = 'verify-lavender-rpm-link-input.py" --pre-lto ltolink.filelist nonltolink.filelist'
    non_lto_count = source.count(non_lto_marker)
    pre_lto_count = source.count(pre_lto_marker)
    if non_lto_count == 1 and pre_lto_count == 1:
        print("PASS: Lavender RPM pre-link guards already installed (idempotent)")
        return
    if non_lto_count or pre_lto_count:
        raise SystemExit("partial/duplicate Lavender RPM pre-link guard injection detected")

    source = replace_once(source, NON_LTO_OLD, NON_LTO_NEW, "non-LTO final-link")
    source = replace_once(source, PRE_LTO_OLD, PRE_LTO_NEW, "PRE_LTO final-link")
    if source.count(non_lto_marker) != 1 or source.count(pre_lto_marker) != 1:
        raise SystemExit("RPM pre-link guard injection did not produce exactly one hook per linker path")
    path.write_text(source)
    print("PASS: installed Lavender RPM object/symbol checks immediately before both XNU final-link paths")


if __name__ == "__main__":
    main()
