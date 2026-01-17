## USBUART
 A cross-platform library for reading/wring data via USB-UART adapters

### Introduction

`USBUART` is a LIBUSB based library that implements a relay from USB-UART 
converter’s endpoints to a pair of I/O resources, either appointed by given 
file descriptors, or created inside the library with pipe[(2)](http://linux.die.net/man/2/pipe).

User application may then use standard I/O operations for reading and writing data.
USBUART Library provides API for three languages - C++, C and Java.

See C/C++ API description in [usbuart.h](usbuart_8h.html) and Android API in 

### Usage with C++

	// Instantiate a context
	context ctx;
	
	// Attach USB via a pipe channel
	channel chnl;
	ctx.pipe(device_id{0x067b,0x0609},chnl,_115200_8N1n);
	
	//Run loop in one thread
	while(ctx.loop(10) >= -error_t::no_channel);
	
	//Read/write data in other thread(s)
	char buff[256];
	read(chnl.fd_read, buff, sizeof(buff));
	
	//or use non-blocking I/O in the loop body

### Termux Support for USB-Serial Pseudo-terminal 

This functionality allows a USB-serial device connected to an Android phone to be accessed by applications running within a Termux environment. It works by creating a pty in Termux that acts as a bridge to the actual USB-serial device.

**Key Component:**

The core of this feature is the `ptyserial` utility, found in the `examples/` directory.

**Prerequisites for Building:**

*   A Termux environment on your Android device.
*   Required packages installed in Termux: `clang`, `libusb` (which provides `libusb-1.0-dev` equivalent functionality in Termux), `pkg-config`, and `make`.
    ```bash
    pkg install clang libusb make pkg-config
    ```

**Prerequisites for Running:**

*   The Termux:API application must be installed on your Android device (this provides the `termux-usb` utility).
*   A USB-to-serial adapter (e.g., CH340, CP210x, FTDI) connected to the Android device via a USB OTG adapter if necessary.

**Building `ptyserial`:**

1.  Follow the main "Building" instructions below to clone the `usbuart` repository and its `libusb` submodule (steps 1-5, ensuring `libusb` is configured and built for your host system if you are cross-compiling, or just built within Termux).
2.  Build the main `libusbuart.so` shared library:
    ```bash
    make all 
    ```
    (or simply `make`)
3.  Build the `ptyserial` utility:
    ```bash
    make ptyserial
    ```
    The binary `ptyserial` will be created in the root directory of the project.

**Running `ptyserial`:**

1.  Identify your USB device: Open Termux and run `termux-usb -l`. This will list connected USB devices. Note the system path for your USB-serial adapter (e.g., `/dev/bus/usb/001/002`).
2.  Execute the bridge utility from the root of the `usbuart` project directory:
    ```bash
    termux-usb -R './ptyserial [command line]' /dev/bus/usb/001/002
    ```
    *   Replace `/dev/bus/usb/001/002` with the actual device path obtained in the previous step.
    *   Replace `[command line]` with your desired full path for the command to run.

    The program will create a pty linked to the USB device listed and execute the command in a child process with stdio connected to the new pty.


