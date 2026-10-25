#!/usr/bin/env python3
"""Build and boot the actual RISC-V kernel; fail on timeout or panic."""
import argparse
import os
from pathlib import Path
import select
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
KERNEL = ROOT / 'assighnment 3' / 'assighnment 3' / 'xv6-riscv-riscv'
p = argparse.ArgumentParser()
p.add_argument('--cpus', type=int, default=3)
p.add_argument('--usertests', action='store_true')
args = p.parse_args()
subprocess.run(['make', '-j2', 'kernel/kernel', 'fs.img'], cwd=KERNEL, check=True)
command = [os.environ.get('QEMU_SYSTEM_RISCV64', 'qemu-system-riscv64'),
    '-machine','virt','-bios','none','-kernel','kernel/kernel','-m','128M',
    '-smp',str(args.cpus),'-display','none','-monitor','none','-serial','stdio',
    '-global','virtio-mmio.force-legacy=false',
    '-drive','file=fs.img,if=none,format=raw,id=x0,snapshot=on',
    '-device','virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0']
proc = subprocess.Popen(command,cwd=KERNEL,stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,stderr=subprocess.STDOUT,bufsize=0)
transcript = bytearray()

cursor = 0
def expect(marker, timeout=40):
    global cursor
    deadline = time.monotonic()+timeout
    while True:
        current = bytes(transcript[cursor:])
        if b'panic:' in current or b'SHMEM FAIL' in current or b'LOG_TEST: FAIL' in current or b'SOME TESTS FAILED' in current:
            raise RuntimeError('kernel/test failure')
        found = current.find(marker)
        if found >= 0:
            cursor += found + len(marker)
            return
        remaining = deadline-time.monotonic()
        if remaining <= 0:
            raise TimeoutError(f'timed out waiting for {marker!r}')
        ready,_,_ = select.select([proc.stdout],[],[],min(.2,remaining))
        if ready:
            chunk = os.read(proc.stdout.fileno(),65536)
            if not chunk:
                raise RuntimeError(f'QEMU exited with {proc.poll()}')
            transcript.extend(chunk)
            sys.stdout.buffer.write(chunk); sys.stdout.buffer.flush()

try:
    expect(b'$ ')
    for command, marker in [('shmem_test',b'SHMEM_REGRESS: PASS'),('log_test',b'LOG_TEST: PASS')]:
        proc.stdin.write((command+'\n').encode()); proc.stdin.flush()
        expect(marker)
        expect(b'$ ')
    if args.usertests:
        proc.stdin.write(b'usertests -q\n'); proc.stdin.flush()
        expect(b'ALL TESTS PASSED',timeout=240)
    print(f'\nQEMU_REGRESS: PASS cpus={args.cpus}')
finally:
    proc.terminate()
    try: proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill(); proc.wait()
