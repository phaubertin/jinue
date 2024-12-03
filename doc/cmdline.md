# Kernel Command Line

## Command Line Syntax

The syntax of the command line is very similar to that of the
[Linux kernel command line](https://github.com/torvalds/linux/blob/master/Documentation/admin-guide/kernel-parameters.rst).
This is on purpose, so the Jinue microkernel can be loaded by boot loaders
intended for Linux.

More specifically:

* The command line is comprised of parameters separated by any combination of
one or more space and/or horizontal tab characters.
* Before the first `--` is encountered (or all parameters if there is no `--` on
the command line):
    * Any parameter that contains an equal sign (e.g. `foo=bar`) and is not
      in double quotes (e.g. not `"foo=bar"`) is considered an option, where the
      part before the first equal sign is the option name and the part after is
      the option value.
    * If the kernel recognizes the option name (see
      [Supported Kernel Options](#supported-kernel-options)), the option is
      treated as a kernel option. Otherwise, it is passed as an environment
      variable to the user space loader and the initial user process. Kernel
      options are not passed to user space as environment variables.
    * Any parameter that is not an option (i.e. that does not contain an equal
      sign and/or is quoted) is a command line argument passed to the user space
      loader and the initial user process.
* After `--`, all parameters are command line arguments for the user space
loader and the initial user process, whether they contain an equal sign or not.
* Option values (but not option names) may be quoted with double quote
characters (e.g. `foo="bar"`), and they must be if they contain space and/or tab
characters (e.g. `foo="bar baz"`).
* Kernel option names and values are case sensitive (e.g. `vga_enable=1` is
different from `VGA_ENABLE=1`).
* In kernel option names and values, underscores (`_`) and dashes (`-`) are
  treated as equivalent (e.g. `vga_enable=1` is the same as `vga-enable=1`).
* If a kernel option is specified multiple times, the last instance has priority.
For user space environment variables, the behaviour is undefined.

## Supported Kernel Options

| Name                | Type    | Description                                                  |
|---------------------|---------|--------------------------------------------------------------|
| `init`              | string  | Path to initial program in initial RAM disk                  |
| `on_panic`          | string  | Action to take after a kernel panic                          |
| `pae`               | string  | Controls whether Physical Address Extension (PAE) is enabled |
| `serial_enable`     | boolean | Enable/disable logging on the serial port                    |
| `serial_baud_rate`  | integer | Baud rate for serial port logging                            |
| `serial_ioport`     | integer | I/O port address for serial port logging                     |
| `serial_dev`        | string  | Serial port device used for logging                          |
| `vga_enable`        | boolean | Enable/disable logging to video (VGA)                        |

For boolean options:

* Values `true`, `enable`, `yes` and `1` are recognized a true.
* Values `false`, `disable`, `no` and `0` are considered false.
* Anything else is invalid.

Integer option values can be specified as a decimal number without any leading
zero (e.g. `42` but not `042`) or as an hexadecimal number prepended with `0x`
(e.g. `0x3f8`).

### Initial Program Path - `init`

Path to initial program in the extracted initial RAM disk.

Type: string

The user space loader loads this executable binary as the initial process. It
must be an ELF binary.

The default for this option is `/sbin/init`.

### Kernel Panic Action - `on_panic`

Action to take after a kernel panic.

Type: string

The following values are recognized:

* `halt` (default) halt CPU.
* `reboot` reboot the system.

Rebooting after a kernel panic can be helpful when testing the kernel in a virtual
machine, as some virtual machine hosts can be set to exit when a reboot is triggered.
If the test hangs instead of exiting, it can cause issues, particularly in automated
testing environments.

### Physical Address Extension - `pae`

Controls whether Physical Address Extension (PAE) is enabled or not.

Type: string

The following values are recognized:

* `auto` (default) PAE is enabled if supported by the CPU, disabled otherwise.
* `disable` PAE is disabled.
* `require` PAE is enabled if supported by the CPU. If not supported, the kernel
  refuses to boot.

This option has security implications: the No eXecute (NX) flag that allows or
prevents code execution from individual memory pages is only supported when PAE
is enabled. For this reason, the kernel logs a warning during boot if PAE is
disabled. If per-page code execution prevention is a security requirement for
you, set this option to `require`.

### Enable Serial Logging - `serial_enable`

Enable/disable logging to a serial port.

Type: boolean

The default for this option is `false` (i.e. disabled).

### Serial Port Baud Rate - `serial_baud_rate`

Baud rate for serial port logging

Type: integer

The following baud rates are supported: `300`, `600`, `1200`, `2400`, `4800`,
`9600`, `14400`, `19200`, `38400`, `57600` and `115200` (default).

### Serial I/O Port - `serial_ioport`

I/O port address for serial port logging

Type: integer

This option allows an unstandard I/O port to be used. If you intend to use a
standard I/O port, it is best to use the `serial_dev` option intead.

If this and the `serial_dev` options are both specified, the last one on the
command line has priority.

### Serial Device - `serial_dev`

Serial port device used for logging 

Type: string 

Use this option if you intend to use a standard PC serial port. If you need to
use a nonstandard I/O port, use the `serial_ioport` option instead.

* The following values are equivalent and select the first serial port (I/O
  address 0x3f8 - default): `0`, `ttyS0`, `/dev/ttyS0`, `com1` and `COM1`.
* The following values are equivalent and select the second serial port (I/O
  address 0x2f8): `1`, `ttyS1`, `/dev/ttyS1`, `com2` and `COM2`.
* The following values are equivalent and select the third serial port (I/O
  address 0x3e8): `2`, `ttyS2`, `/dev/ttyS2`, `com3` and `COM3`.
* The following values are equivalent and select the fourth serial port (I/O
  address 0x2e8): `3`, `ttyS3`, `/dev/ttyS3`, `com4` and `COM4`.

If this and the `serial_ioport` options are both specified, the last one on the
command line has priority.

### Enable Logging to VGA Video - `vga_enable`

Enable/disable logging to video (VGA)

Type: boolean

The default for this option is `true` (i.e. enabled).
