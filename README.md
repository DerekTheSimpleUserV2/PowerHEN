# PowerHEN

A lightweight, hybrid Payload and Homebrew Enabler framework written in pure C/C++ for PlayStation 4 and PlayStation 5 systems. 

**PowerHEN** is designed from scratch without standard computer libraries (`-nostdlib`) to ensure execution independent of position (PIC), delivering optimized kernel infrastructure and stable network payloads.

## Features

- **Hybrid Hardware Detection:** Automatically identifies whether it is running under PS4 (Orbis OS) or PS5 (Prosper0 OS) architecture using native FreeBSD system checks.
- **Integrated Payload Server:** Spawns a lightweight TCP network socket listening on port `9025` to safely stage, mprotect (RWX), and execute secondary payload binaries directly into RAM.
- **Native User Interface Feedback:** Uses a clean, isolated system notification system (`/dev/notification0`) to report execution status directly to the screen via "PowerHEN Loaded Successfully" alerts without external dependencies.
- **Flexible Entry Points:** Tailored for integration with low-level network injection protocols (like PPPwn) or standard webkit sandbox escapes.

## Technical Requirements & Compiling

The project uses raw instruction mapping and custom inline assembly to invoke system calls directly to the kernel, preventing freezes and panics caused by version offsets.

### Prerequisites (Linux Mint / Ubuntu)
Install the standard 64-bit cross-compiler toolchain:
```bash
sudo apt update && sudo apt install build-essential gcc-x86-64-linux-gnu -y
```

### Direct Compilation
To compile your `main.c` file directly into the flat executable binary (`powerhen_v0.1.bin`) without a Makefile, run the following command in your terminal:

```bash
x86_64-linux-gnu-gcc -m64 -fPIC -fno-builtin -nostdlib -fno-stack-protector -O2 -fdata-sections -ffunction-sections -c main.c -o main.o && x86_64-linux-gnu-ld -Ttext 0x0 -entry=_start -o powerhen.elf main.o && x86_64-linux-gnu-objcopy -O binary -j .text -j .rodata powerhen.elf powerhen_v0.1.bin && rm main.o powerhen.elf
```

## Usage

### USB Deployment
1. Format your external storage drive to **exFAT** or **FAT32**.
2. Copy `powerhen_v0.1.bin` into the root directory of the drive.
3. Plug the drive into your console and launch your local menu's USB Payload Loader.

### Network Deployment
Transmit the raw binary sequentially to your console's staging port (e.g., `9020` or `9023`) using netcat:
```bash
nc -w 3 <your_console_ip> <port> < powerhen_v0.1.bin
```

---
*Disclaimer: This project is created for educational research regarding hardware architecture, embedded systems memory management, and POSIX compliance testing.*
