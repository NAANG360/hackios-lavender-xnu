#!/usr/bin/env python3
"""Key-only SSH over the USB network link, installed into a mounted iOS root.

  sudo sweet-ssh.py MOUNTED_ROOT SSH_OUT PUBKEY_FILE [PUBKEY_FILE...]

SSH_OUT holds the binaries built by bergamota's tools/ssh/build.sh (dropbear
2026.94, dash 0.5.13.5, toybox 0.8.15 for arm64-apple-ios16.0, ad hoc signed
with platform-application/no-sandbox entitlements) plus their macOS builds
dropbear-mac and toybox-mac (host keys, applet list). PUBKEY_FILE: public keys
allowed to log in as root (never a private key). Idempotent. Installs:

  /usr/local/bin  dropbear, dropbearkey and scp (links), toybox + applet links
  /bin/sh         dash (dropbear runs the login shell and scp through it)
  /private/etc    shells += /bin/sh; dropbear/ host keys (ed25519, ecdsa)
  /var/root/.ssh  authorized_keys (root 0700/0600)
  launchd         org.sweet.dropbear: dropbear -F -E -s -p 22, KeepAlive, both
                  as a LaunchDaemons plist and in xpc/launchd.plist (the cache
                  launchd reads with launchd_unsecure_cache=1)
  configd         static ue0 service 192.168.7.2/24 via 192.168.7.1, so
                  configd keeps the address the kernel (patch 0213) set
  bindfs dirs     directories BindFS / factory-data clients expect on a real
                  data volume (missing ones logged on every boot)
"""
import glob, os, plistlib, shutil, subprocess, sys, tempfile

LABEL = 'org.sweet.dropbear'
HOSTKEY = '/private/etc/dropbear/dropbear_ed25519_host_key'


def load(path):
    with open(path, 'rb') as f:
        return plistlib.load(f)


def save(path, data, uid, gid, mode, fmt=plistlib.FMT_BINARY):
    with open(path, 'wb') as f:
        plistlib.dump(data, f, fmt=fmt)
    os.chown(path, uid, gid)
    os.chmod(path, mode)


def install_file(src, dst, mode=0o755):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    tmp = dst + '.new'
    shutil.copyfile(src, tmp)
    os.chown(tmp, 0, 0)
    os.chmod(tmp, mode)
    os.replace(tmp, dst)


def relink(target, link):
    if os.path.islink(link) and os.readlink(link) == target:
        return
    if os.path.lexists(link):
        os.remove(link)
    os.symlink(target, link)


def mkdirs(path, uid=0, gid=0, mode=0o755):
    if not os.path.isdir(path):
        os.makedirs(path)
        os.chown(path, uid, gid)
        os.chmod(path, mode)
        return True
    return False


def binaries(root, out):
    bindir = os.path.join(root, 'usr/local/bin')
    install_file(os.path.join(out, 'dropbear'), os.path.join(bindir, 'dropbear'))
    for n in ('dropbearkey', 'scp'):
        relink('dropbear', os.path.join(bindir, n))
    install_file(os.path.join(out, 'toybox'), os.path.join(bindir, 'toybox'))
    applets = subprocess.run([os.path.join(out, 'toybox-mac')], capture_output=True,
                             text=True, check=True).stdout.split()
    n = 0
    for a in applets:
        dst = os.path.join(bindir, a)
        if a != 'toybox' and (not os.path.lexists(dst) or os.path.islink(dst)):
            relink('toybox', dst)
            n += 1
    sh = os.path.join(root, 'bin/sh')
    if os.path.exists(sh) and not os.path.exists(sh + '.orig'):
        shutil.copy2(sh, sh + '.orig')
    install_file(os.path.join(out, 'dash'), sh)
    print('ssh: /usr/local/bin/{dropbear,dropbearkey,scp,toybox + %d applets}, /bin/sh = dash' % n)

    shells = os.path.join(root, 'private/etc/shells')
    lines = open(shells).read().splitlines() if os.path.exists(shells) else []
    if '/bin/sh' not in lines:
        with open(shells, 'w') as f:
            f.write('\n'.join(lines + ['/bin/sh']) + '\n')
        os.chown(shells, 0, 0)
        os.chmod(shells, 0o644)
        print('ssh: /private/etc/shells += /bin/sh')

    passwd = os.path.join(root, 'private/etc/passwd')
    entry = next((l for l in open(passwd).read().splitlines() if l.startswith('root:')), None) \
        if os.path.exists(passwd) else None
    if entry is None or entry.split(':')[-2:] != ['/var/root', '/bin/sh']:
        print('warning: root entry in /private/etc/passwd is not ...:/var/root:/bin/sh:', entry)


def authorized_keys(root, keyfiles):
    keys = []
    for p in keyfiles:
        line = open(p).read().strip().splitlines()[0]
        if not line.split()[0].startswith(('ssh-', 'ecdsa-', 'sk-')) or 'PRIVATE' in line:
            sys.exit('not a public key: %s' % p)
        keys.append(line)
    d = os.path.join(root, 'private/var/root/.ssh')
    os.makedirs(d, exist_ok=True)
    os.chown(d, 0, 0)
    os.chmod(d, 0o700)
    ak = os.path.join(d, 'authorized_keys')
    have = open(ak).read().splitlines() if os.path.exists(ak) else []
    for k in keys:
        if k not in have:
            have.append(k)
    with open(ak, 'w') as f:
        f.write('\n'.join(have) + '\n')
    os.chown(ak, 0, 0)
    os.chmod(ak, 0o600)
    print('ssh: authorized_keys has %d key(s)' % len(have))


def host_keys(root, out):
    kd = os.path.join(root, 'private/etc/dropbear')
    os.makedirs(kd, exist_ok=True)
    os.chown(kd, 0, 0)
    os.chmod(kd, 0o700)
    with tempfile.TemporaryDirectory() as tmp:
        tool = os.path.join(tmp, 'dropbearkey')
        os.symlink(os.path.join(out, 'dropbear-mac'), tool)   # multi-call binary: argv[0] picks it
        for typ in ('ed25519', 'ecdsa'):
            kf = os.path.join(kd, 'dropbear_%s_host_key' % typ)
            if os.path.exists(kf):
                print('ssh: host key kept', os.path.basename(kf))
                continue
            r = subprocess.run([tool, '-t', typ, '-f', kf], check=True, capture_output=True, text=True)
            os.chown(kf, 0, 0)
            os.chmod(kf, 0o600)
            fp = [l for l in r.stdout.splitlines() if 'Fingerprint' in l]
            print('ssh: host key generated', os.path.basename(kf), *(fp[:1]))


def launchd_job(root):
    job = {'Label': LABEL,
           'ProgramArguments': ['/usr/local/bin/dropbear', '-F', '-E', '-s', '-p', '22', '-r', HOSTKEY],
           'RunAtLoad': True, 'KeepAlive': True, 'UserName': 'root',
           'StandardErrorPath': '/dev/console'}
    jpath = '/System/Library/LaunchDaemons/%s.plist' % LABEL
    os.makedirs(os.path.join(root, os.path.dirname(jpath).lstrip('/')), exist_ok=True)
    save(os.path.join(root, jpath.lstrip('/')), job, 0, 0, 0o644, plistlib.FMT_XML)
    lpath = os.path.join(root, 'System/Library/xpc/launchd.plist')
    st = os.stat(lpath)
    plist = load(lpath)
    plist['LaunchDaemons'][jpath] = job
    save(lpath, plist, st.st_uid, st.st_gid, st.st_mode & 0o7777)
    print('ssh: launchd job', LABEL, '(port 22, key-only)')


def usbnet(root):
    base = os.path.join(root, 'private/var/preferences/SystemConfiguration')
    os.makedirs(base, exist_ok=True)
    svc, nset = '5EE7C0DE-0000-4000-8000-0000000000E0', '5EE7C0DE-0000-4000-8000-0000000005E7'
    path = os.path.join(base, 'preferences.plist')
    prefs = load(path) if os.path.exists(path) else {'__VERSION__': 20191120}
    prefs.setdefault('NetworkServices', {})[svc] = {
        'UserDefinedName': 'USB Ethernet',
        'Interface': {'DeviceName': 'ue0', 'Hardware': 'Ethernet', 'Type': 'Ethernet',
                      'UserDefinedName': 'USB Ethernet'},
        'IPv4': {'ConfigMethod': 'Manual', 'Addresses': ['192.168.7.2'],
                 'SubnetMasks': ['255.255.255.0'], 'Router': '192.168.7.1'},
        'IPv6': {'ConfigMethod': 'Automatic'},
        'DNS': {'ServerAddresses': ['1.1.1.1', '8.8.8.8']},
    }
    sets = prefs.setdefault('Sets', {})
    cur = prefs.get('CurrentSet', '').rsplit('/', 1)[-1] or nset
    s = sets.setdefault(cur, {'UserDefinedName': 'Automatic', 'Network': {}})
    net = s.setdefault('Network', {})
    net.setdefault('Service', {})[svc] = {'__LINK__': '/NetworkServices/' + svc}
    order = net.setdefault('Global', {}).setdefault('IPv4', {}).setdefault('ServiceOrder', [])
    if svc not in order:
        order.insert(0, svc)
    net.setdefault('Interface', {})
    prefs['CurrentSet'] = '/Sets/' + cur
    save(path, prefs, 0, 0, 0o644)

    path = os.path.join(base, 'NetworkInterfaces.plist')
    ifs = load(path) if os.path.exists(path) else {'__VERSION__': 20191120}
    lst = [i for i in ifs.get('Interfaces', []) if i.get('BSD Name') != 'ue0']
    lst.append({'BSD Name': 'ue0', 'IOBuiltin': True, 'IOInterfaceNamePrefix': 'ue',
                'IOInterfaceType': 6, 'IOInterfaceUnit': 0,
                'IOMACAddress': bytes.fromhex('02005e77ee02'),   # ud_phone_mac (patch 0075)
                'SCNetworkInterfaceType': 'Ethernet',
                'SCNetworkInterfaceInfo': {'UserDefinedName': 'USB Ethernet'}})
    ifs['Interfaces'] = lst
    save(path, ifs, 0, 0, 0o644)
    print('usbnet: configd service ue0 192.168.7.2/24 via 192.168.7.1')


def bindfs_dirs(root):
    made = []
    for rel in ('private/var/hardware/Pearl/System/Library/Pearl/ReferenceFrames',
                'private/var/hardware/FactoryData/System/Library/Caches/com.apple.factorydata'):
        if mkdirs(os.path.join(root, rel)):
            made.append(rel)
    pre = [p for p in glob.glob(os.path.join(root, 'private/preboot/*')) if os.path.isdir(p)]
    for p in pre:
        if mkdirs(os.path.join(p, 'usr/standalone/firmware')):
            made.append(os.path.relpath(p, root) + '/usr/standalone/firmware')
    if not pre:
        print('bindfs: no /private/preboot/<hash> directory, firmware dir skipped')
    print('bindfs: created', ', '.join(made) if made else 'nothing (all present)')


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    if os.geteuid() != 0:
        sys.exit('run as root (sudo)')
    root, out, keys = sys.argv[1], sys.argv[2], sys.argv[3:]
    if not os.path.exists(os.path.join(root, 'System/Library/xpc/launchd.plist')):
        sys.exit('not an iOS root: %s' % root)
    need = ('dropbear', 'dash', 'toybox', 'dropbear-mac', 'toybox-mac')
    missing = [n for n in need if not os.path.isfile(os.path.join(out, n))]
    if missing:
        sys.exit('missing in %s: %s (run tools/ssh/build.sh)' % (out, ', '.join(missing)))
    binaries(root, out)
    authorized_keys(root, keys)
    host_keys(root, out)
    launchd_job(root)
    usbnet(root)
    bindfs_dirs(root)
    print('SWEET_SSH_DONE')


if __name__ == '__main__':
    main()
