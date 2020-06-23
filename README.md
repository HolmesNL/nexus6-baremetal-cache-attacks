# A bare-metal environment for cache side-channel attacks

Based on https://github.com/zhuowei/nexus7-baremetal

- Uses code from https://github.com/IAIK/armageddon/blob/master/libflush
- Includes an AES and RSA implementation from OpenSSL
- Uses https://github.com/kokke/tiny-AES-c

## Usage

```
make
fastboot boot kernel.img
```

Then connect via a UART interface and send a character (p,c,e,f etc.) to execute the relevant section of `bootloader05.c`.
