# kernel_samsung_mt6768-lucreticus

Lucreticus kernel for Samsung Galaxy A32 (SM-A325F, MT6768 / MT6769) and siblings. Based on Samsung `4.14.357` (OpenELA `4.14.357-openela`), rebranded from slmkernel to **lucreticus `r1-Armaros`** (`-r1-Armaros`, `CONFIG_LOCALVERSION="-lucreticus_r1-Armaros@dotarma"`, `uname -r` spoofed to `5.10.239`).

Source: `stardustps/kernel_samsung_mt6768-lucreticus` (`rc1` branch, fork of `Samsung-MT6769-Devs/android_kernel_samsung_mt6768`).

## Devices

- `a32` — Galaxy A32 (k69v1_64_titan_marmot)
- `a22` — Galaxy A22 (k69v1_64_titan_buffalo)
- `f22` / `m22` / `m32` — experimental, same SoC family
- Common base: `arch/arm64/configs/mt6768_lucreticus_defconfig` + `a32.config` / `a22.config` / `f22.config` / `m22.config` / `m32.config` + `perf.config` / `battery.config` merged to `compiled_defconfig` at build time.

## Features

- **Rebrand** — `slmkernel` → `lucreticus` (`SLMKERNEL` → `LUCRETICUS`), `build_slmkernel.sh` → `build_lucreticus.sh`, `mt6768_slm_defconfig` → `mt6768_lucreticus_defconfig`, `CONFIG_LOCALVERSION`, `EXTRAVERSION=-r1-Armaros`.
- **Nomount** — `fs/nomount.{c,h}` hides sensitive mountpoints from `/proc/{mounts,mountinfo,mountstats}` via `do_mount`/`do_umount` checks and `proc_namespace` filtering, `CONFIG_NOMOUNT=y`, toggle `/proc/nomount_enabled`.
- **BBRv2** — backported from `DPR-KernelArchive/sweetie_star_kernel_xiaomi_sweet` (`sixteen-qpr1`, `RainyXeon <rainyxeon@gmail.com>`), keeps BBRv1 in `tcp_bbr.c` and adds BBRv2 as `tcp_bbr2.c` (`TCP_CONG_BBR2`, `DEFAULT_BBR2`), `CONFIG_TCP_CONG_BBR2=y`.
- **ReSukiSU** — the optional CI setup uses its `main` branch with `CONFIG_KSU_MANUAL_HOOK=y` and disables `CONFIG_KSU_SUSFS` to avoid SUSFS inline hook mode. The existing manual hooks are in `fs/exec.c`, `fs/open.c`, and `fs/stat.c`. NoMount remains independently controlled by `CONFIG_NOMOUNT`.
- **Uname spoof** — `Makefile:KERNELVERSION = 5.10.239` so userspace `uname -r` reports `5.10.239` (spoofed), internal `VERSION/PATCHLEVEL/SUBLEVEL/EXTRAVERSION` kept for module deps.
- **Droidspaces** — container prerequisites enabled in base defconfig: `SYSVIPC, POSIX_MQUEUE, IPC_NS, USER_NS, CGROUP_NET_PRIO, DEVTMPFS, TMPFS_POSIX_ACL/XATTR, NF_TABLES, NETFILTER_XT_MATCH_ADDRTYPE`.
- **Aigis** — VoLTE IPv6 `ip6_output` cork fix retained, Mali Valhall `r32p1` pinned (`CONFIG_MTK_GPU_VERSION="mali valhall r32p1"`).
- **Zen I/O scheduler** — `block/zen-iosched.c` (FCFS + deadlines, `sync_expire=HZ/2`, `async_expire=5*HZ`), `IOSCHED_ZEN` / `DEFAULT_ZEN`.
- **Dynamic fsync (experimental)** — defers writable regular-file `fsync`/`fdatasync` while the display is on, then schedules a sync five seconds after the first deferral, at display blank, and at suspend. It is controlled by `CONFIG_DYNAMIC_FSYNC` and the kernel-manager-compatible `/sys/kernel/dyn_fsync/Dyn_fsync_active`. A crash before the next sync can lose recent writes.
- **OLED burn-in profile (experimental)** — caps normal brightness on the A32/A22/M22/M32 Samsung OLED panels (including F22 through its M22 config) at level 220 by default (`CONFIG_LUCRETICUS_BURNIN_PROTECTION`). AOD and the A32 fingerprint mask path are left to the panel driver. Runtime parameters are `/sys/module/lucreticus_burnin/parameters/enabled` and `max_level` (1–255); changes take effect on the next brightness update.
- **Simple GPU Algorithm / Mali touch boost (experimental)** — optional GED tuning for the MT6768 Mali GPU. `MTK_SIMPLE_GPU_ALGORITHM` biases high-load requests up one OPP and holds against rapid downscaling; `MTK_MALI_BOOST` adapts AdrenoBoost-style touch requests to Mali, with level 0–3 through `/sys/module/ged/parameters/mali_boost_level`. GED's customization and thermal ceilings remain active.
- **MediaTek bus boost (experimental)** — `MTK_DEVFREQ_BUS_BOOST` requests DDR OPP 1 for 100 ms at the start of a touch through MediaTek DVFSRC PM QoS, avoiding repeated requests on every movement. Runtime parameters are `enabled`, `boost_opp` (0–2), and `duration_ms` (20–1000) under `/sys/module/mtk_bus_boost/parameters/`.
- **SchedTune/uclamp tuning (experimental)** — `LUCRETICUS_SCHED_TUNING` extends the SchedTune boost hold from 50 to 80 ms and starts the global uclamp minimum at 64/1024. The existing cgroup and `/proc/sys/kernel/sched_uclamp_util_min` controls can change tuning after boot.
- **PD and QC/AFC charge profile (experimental)** — `LUCRETICUS_FAST_CHARGE_PROFILE` prefers an advertised fixed USB-PD profile at or below 9 V with the highest available current, falls back to 5 V if no suitable higher profile exists, and requests 9 V rather than 12 V for QC/AFC high-voltage charging. It does not raise charger, cable, battery, or thermal limits; actual current depends on the source and the Samsung battery votes.
- **Audio-jack consumer IR (experimental)** — `tools/lucreticus/audio_jack_ir.py` produces a 48 kHz stereo NEC waveform for a *separate* audio-jack IR LED emitter. Run `python3 tools/lucreticus/audio_jack_ir.py 0x20DF10EF power.wav`, copy the WAV to the phone, and play it through the wired output with a suitable adapter. The workflow option places the tool in `extras/` inside the zip for manual extraction. This transmits consumer IR only; it does not provide IrDA networking or an IR receiver.
- **Bluetooth HCI trace and transport (experimental)** — `MTK_BT_HCI_TRACE` adds a `mtk_bt:mtk_bt_hci` tracefs event on `/dev/stpbt` traffic. Enable it only during capture because HCI payloads may contain private data. `MTK_BT_AUDIO_TRANSPORT` raises the driver HCI buffer from 2048 to 4096 bytes. A2DP codec choice and bitrate are negotiated by the Android Bluetooth stack and the headset, so the kernel transport option cannot force a codec bitrate.
- **WoWLAN link retention (experimental)** — `MTK_WLAN_WOWLAN_KEEPALIVE` adds disconnect and beacon-loss wake triggers and permits ARP in the existing MediaTek WoWLAN firmware mode. The existing Wi-Fi vendor keep-alive command still needs a userspace request and supported firmware; enabling the build option alone does not schedule periodic packets.
- **AIO** — gated completion wakeups (`wait_min_nr` / `last_wakeup_completed`) and acquire/release for `ring->tail`, `CONFIG_AIO_OPTIMIZE=y`.
- **Bypass charging** — `CONFIG_MTK_BYPASS_CHARGING` (mediatek), `sysfs` `/sys/kernel/bypass_charging/bypass_charging` and `bypass_charging` module param, `_mtk_charger_do_charging` suppresses charging when enabled.
- **WireGuard** — `wireguard-linux-compat` via `kernel-tree-scripts/jury-rig.sh` at build time, `CONFIG_WIREGUARD` + `NET_UDP_TUNNEL/DST_CACHE/CRYPTO_ALGAPI`, compat `__kernel_timespec` guarded for this tree's `time64.h` backport.
- **Docker/LXC** — `CFS_BANDWIDTH, CGROUP_HUGETLB, NET_CLS_CGROUP, MACVLAN, VXLAN, BRIDGE_VLAN_FILTERING, BTRFS_FS`.
- **LTO** — `LTO_CLANG` + `THINLTO` (`-flto=thin`, `--thinlto-cache-dir`) or full (`-flto`), `LD_FLAGS_LTO_CLANG=-mllvm -import-instr-limit=5`.
- **Clocks / Tick** — `LUCRETICUS_OC_GPU/CCI/RAMDVFS` vs `LUCRETICUS_UV`, `CONFIG_HZ` (`100/250/300/1000`), `SCHED_BORE` via `perf.config`.
- **Security / Debug strip** — `TZDEV/TEGRIS` and leaf debug (`DYNAMIC_DEBUG, DEBUG_INFO, SCHED_DEBUG, DEBUG_LIST, FTRACE, MAGIC_SYSRQ, KALLSYMS_ALL`) toggles, `SCHED_DEBUG/DEBUG_LIST/MAGIC_SYSRQ` force-selected by `mediatek/Kconfig.default` and survive stripping.
- **CVE backports** — `algif_aead` Copy Fail `CVE-2026-31431` (out-of-place), `raw_send_hdrinc` `CVE-2026-64114` (`ihl<5`), `esp4/esp6` Dirty Frag `CVE-2026-43284` (`SKBTX_SHARED_FRAG` / `skb_cow_data` fallback), `netprio` `css->id` vs removed `cgroup->id`.
- **Netprio fix** — `task_netprioidx` / `netprio_cgroup` use `css->id`.
- **AnyKernel3** — `stardustps/sta7dust` packaging (`Image`→`Image.gz`, flashable zip).

## How to build locally

### Toolchain

ZyC Clang 14, e.g. `https://github.com/ZyCromerZ/Clang/releases/download/14.0.6-20250704-release/Clang-14.0.6-20250704.tar.gz`:

```bash
mkdir -p ~/zyc-clang
tar -xf Clang-14.0.6-20250704.tar.gz -C ~/zyc-clang
export TC=~/zyc-clang
export CROSS_COMPILE=$TC/bin/aarch64-linux-gnu-
export CROSS_COMPILE_ARM32=$TC/bin/arm-linux-gnueabi-
export LD=$TC/bin/ld.lld
export CC=$TC/bin/clang
export ARCH=arm64
```

Dependencies (host):

```bash
sudo apt update
sudo apt install -y \
  build-essential bc bison flex patch pkg-config git curl tar xz-utils zip unzip \
  cpio rsync kmod perl python3 python-is-python3 libssl-dev libelf-dev pahole lld \
  libncurses-dev zlib1g-dev libyaml-dev lz4 zstd device-tree-compiler adb fastboot
```

### Via helper script

```bash
./build_lucreticus.sh
# prompts: a32/a22/f22/m22/m32, then merges
# mt6768_lucreticus_defconfig + <device>.config + battery.config
# appends: # CONFIG_ALWAYS_ENFORCE is not set, CONFIG_ALWAYS_PERMISSIVE=y,
#          CONFIG_MTK_GPU_VERSION="mali valhall r32p1"
# runs: make O=out compiled_defconfig && make -j$(nproc) -C out
```

Edit the script to use `perf.config` for OC or add `ksu.config` for KernelSU.

### Manual

```bash
export CFGDIR=arch/arm64/configs
cat $CFGDIR/mt6768_lucreticus_defconfig $CFGDIR/a32.config > $CFGDIR/compiled_defconfig
# optional: cat $CFGDIR/perf.config >> $CFGDIR/compiled_defconfig
# optional: cat $CFGDIR/ksu.config >> $CFGDIR/compiled_defconfig  # after KernelSU setup.sh
# optional toggles via scripts/config:
# scripts/config --file $CFGDIR/compiled_defconfig --enable CONFIG_IOSCHED_ZEN
# scripts/config --file $CFGDIR/compiled_defconfig --enable CONFIG_MTK_BYPASS_CHARGING
echo '# CONFIG_ALWAYS_ENFORCE is not set' >> $CFGDIR/compiled_defconfig
echo 'CONFIG_ALWAYS_PERMISSIVE=y' >> $CFGDIR/compiled_defconfig
echo 'CONFIG_MTK_GPU_VERSION="mali valhall r32p1"' >> $CFGDIR/compiled_defconfig
make O=out -j$(nproc) compiled_defconfig
make -s O=out -j$(nproc)
# out/arch/arm64/boot/Image (gzip to Image.gz for AnyKernel3)
```

KernelSU setup (if needed): `curl -LSs "https://raw.githubusercontent.com/ReSukiSU/ReSukiSU/main/kernel/setup.sh" | bash`. Keep `CONFIG_KSU_MANUAL_HOOK=y` and `# CONFIG_KSU_SUSFS is not set` in `ksu.config`.

WireGuard (if enabled in CI): `git clone --depth 1 https://github.com/WireGuard/wireguard-linux-compat.git && ./wireguard-linux-compat/kernel-tree-scripts/jury-rig.sh $(pwd)`.

## How to build via GitHub Actions

`Actions → Build Lucreticus Kernel → Run workflow`:

- `device` — `a32/a22/f22/m22/m32/all` (matrix fans out across devices)
- `ksu` — add KernelSU
- `profile` — `perf/balance/battery/all` (`perf` OC + `HZ_250`, `battery` UV + `HZ_100`, `balance` base only)
- `opt` — `O2/O3` (`KCFLAGS/KCPPFLAGS`)
- `droidspaces` / `nomount` / `zen` / `aio_opt` / `wireguard` / `docker` / `bypass_charging`
- `lto` — `none/thin/full`
- `clock` — `stock/overclock/downclock` (`LUCRETICUS_OC_*` / `UV`)
- `hz` — `100/250/300/1000` (`CONFIG_HZ`)
- `gpu_clock` — `stock/overclock/downclock/max` (`LUCRETICUS_OC_GPU`)
- `nosec` / `nodebug` / `use_cache` — experimentals
- `dynamic_fsync` / `burnin` / `simple_gpu` / `mali_boost` / `mtk_bus_boost` — independent experimental feature toggles, off by default
- `fast_charge` — high-current PD / 9 V QC-AFC profile switch, off by default
- `experimental_features` — comma-separated independent flags, with no spaces: `sched_tuning`, `audio_jack_ir`, `bt_hci_snoop`, `bt_audio_transport`, `wowlan_keepalive`. Leave blank to disable all. For example, `sched_tuning,bt_hci_snoop` enables those two only. This shared input keeps the workflow within GitHub's 25-input limit. The workflow checks the generated kernel config and fails if an enabled option is unavailable; cache reuse requires the same source revision and effective feature set.

Build does: deps → ZyC Clang 14 → optional KernelSU/WireGuard → merge defconfigs → `scripts/config` toggles → `make compiled_defconfig` → `make -s -C out -j$(nproc)` → `stardustps/sta7dust` (`Image`→`Image.gz`) → flashable zip `lucreticus-r1-Armaros-<device>-<profile>-<opt>[-ksu][-ds][-nm][-zen][-docker][-bypass][-dfsync][-burnin][-sgpu][-mboost][-busboost][-stune][-fastchg][-irjack][-bttrace][-btbuf][-wowlan][-oc/-uv][-hz][-gpu*][-wg][-thinlto][-cache]-<sha>.zip` → artifact + single Telegram summary (`notify` job, `sendMessage` + per-zip `sendDocument`, guarded against empty artifact set).

Secrets: `TELEGRAM_BOT_TOKEN` / `TELEGRAM_CHAT_ID` for `sendDocument`.

## License

GPL-2.0. See `COPYING`. MediaTek/Samsung downstream files retain their original headers.
