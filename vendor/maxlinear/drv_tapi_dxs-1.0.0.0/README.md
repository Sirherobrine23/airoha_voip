Copyright 2024 MaxLinear, Inc.

For licensing information, see the file [LICENSE](LICENSE) in the root folder of
this software module.

----

# Standalone TAPI for DXS

## USER ADAPTATIONS

For system-specific adaptations, a file named `drv_config_user.h` needs to be
created in the `src/dxs` directory. Pre-configured files for each of the evaluation
boards that use a DUSLIC XS are provided with this driver. For the adaption of
a new board, a file named
[drv_config_user.template.h](src/dxs/drv_config_user.template.h) with blank macros
exists, too. One of these files must be copied into the target build directory
and renamed to `drv_config_user.h`.

The number of DXS devices the driver can manage is configurable. By configuring
the driver to the exact number of  DXS device on your hardware you can save
a little memory and system resources like file descriptors.
Besides the small extra resource usage, there is no disadvantage to configure
the driver for more devices than the HW provides. This way, hardware variants
with a different number of devices can be covered by a single piece of software.
The application would there always try to initialize the maximum number of
devices but only the devices actually present on the hardware will respond.
With this, the service can scale dynamically with just a single software.

To configure the number of DXS devices the driver should support use
`--with-max-devices=<yourNumberOfDevicesToBeSupported>` as an argument in the
configure step. Please replace `<yourNumberOfDevicesToBeSupported>` with a
digit in between 1 and 8.

### Adaptation to systems without Device-tree support

For systems without device tree support user has to handle all pieces of - SPI
interface, chip reset line, interrupts - configuration and initialization by
providing implementation for the following macros:

> Note: example of system which uses device-tree but still makes use of some of
following macros to configure parts of the system can be found in file
[drv_config_user.easy3201.evs.h](src/dxs/drv_config_user.easy3201.evs.h).

* `SPI_INIT(pDev)` - SPI initialization. By default it registers the driver with the Linux SPI framework
  using `DXS_SPI_drvRegister()` function.
* `SPI_EXIT(pDev)` - SPI de-initialization, freeing SPI resources if required. By default
  it releases the driver from Linux SPI framework using `DXS_SPI_drvUnregister()` function.
* `SPI_CS_SET(devNo, high_low)` - setting of an IO-pin to be used as chip select signal (e.g. via direct GPIO manipulation). Define it as blank/dummy macro if SPI framework or hardware can drive CS pin on it's own. This macro can also be used to cover CS pin handling only for particular devices (by checking `devNo` parameter in order to extend SPI bus or hardware possibilities.)
* `DXS_SPI_MODE_SET(pDev, mode)` - setting SPI mode (DXS uses SPI mode 3).
* `DXS_SPI_BAUDRATE_SET(pDev, baudrate)` - setting SPI baudrate.
* `SPI_MAXBYTES_SIZE` - maximum number of bytes that can be transferred via SPI in one go.
* `spi_ll_read_write(pDev,txptr,txsize,rxptr,rxsize)` - SPI low level access function.
  This function should write/read bytes to/from SPI driver.

Reset pin handling:

* `CHIP_RESET(pDev, reset)` - sets/resets DXS reset line (this can be done e.g. via
  direct GPIO manipulation)

IRQ line handling related macros to define:

* `IRQLINE_INIT(pDev)` - configure and enable the interrupt line.
* `IRQLINE_EXIT(pDev)` - disable the interrupt line.

### Adaptation to systems with Device-tree support

Systems which support device-tree will handle most of SPI initialization using
in-built Linux SPI subsystem which should take settings as SPI clock frequency,
mode, clock phase and polarity from device tree node definition. Device tree node
covers also interrupt and reset pin settings.

#### Device-tree sample node definition

A sample device tree node configuration for the Duslic device can be found in
`doc/dxs` directory. See file [lantiq,duslicxs.txt](doc/dxs/lantiq,duslicxs.txt).
By proper device tree node definition, the user specifies SPI interface (pins, clock frequency), reset and
interrupt pins that Duslic device uses for normal operation.

## HOW TO COMPILE

TAPI drivers are built using the commonly used configure, make, make install
process. The first step is to create a Makefile for your specific system
using the provided configure script and adapt the parameters according
to your paths and required features. Most important in this step is to
define the TAPI API version that is used. Use `--enable-tapi3` for CPE products
and `--enable-tapi4` for AN products.

If the Autoconf/Automake tools are not used to configure the driver the file
`include/drv_tapi_if_version.h` has to be generated manually. To facilitate
this two template files are placed in the include subdirectory. Copy one of
these files to `drv_tapi_if_version.h`. Use `drv_tapi_if_version.v3` for CPE
products and `drv_tapi_if_version.v4` for AN products.

For Linux, the build process is controlled by the kbuild system.
The kbuild system knows already the architecture-specific CFLAGS.
The following configure options must be provided:
`--with-kernelincl=<path>`, `--with-kernelbuild=<path>`
If the kernel has been built inside the source directory (which is not nice),
the kernelbuild path is optional.

### Configure TAPI

At first create a separate build directory, e.g. below the sources.

```bash
mkdir build_dir
cd build_dir
```

To check all possible configuration switches `../configure --help` can be called (part of the output):

```bash
$ ./configure --help
'configure' configures Standalone TAPI driver for DXS to adapt to many kinds of systems.

Usage: ./configure [OPTION]... [VAR=VALUE]...

To assign environment variables (e.g., CC, CFLAGS...), specify them as
VAR=VALUE.  See below for descriptions of some of the useful variables.

Defaults for the options are specified in brackets.

Configuration:
  -h, --help              display this help and exit
      --help=short        display options specific to this package
      --help=recursive    display the short help of all the included packages
...
  --enable-tapi4          enable TAPI Version 4 interface (single device node)
                          (disabled by default)
  --enable-tapi3          enable TAPI Version 3 interface (multiple device
                          node) (enabled by default)
  --enable-kernel-api     enable Kernel API (disabled by default)
  --enable-classic-tapi-compat
                          enable classic TAPI driver compatibility (enabled by
                          default)
  --enable-tapi-debug-buffer
                          enable TAPI debug buffer (disabled by default)
...(and so on)
```


>**📝Note:** Selecting the correct DC/DC type  
It is crucial to select a valid DC/DC type name from the following list IBB12, CIBB12, IB12,
CIB12, BB48, CBB48, IFB3, IFB12, CIFB3, CIFB12, IBGD12, IBVD3.
The selected DC/DC type has to match the DC/DC type on your PCB design in order to avoid component damage.

Example `configure` call for ATOM CPU, LGM platform:

```bash
../configure --with-kernel-incl=<your path to linux kernel include directory> --target=x86_64-openwrt-linux --host=x86_64-openwrt-linux --build=x86_64-pc-linux-gnu --enable-trace --enable-debug --enable-proc --enable-cid --enable-dcdc-hw=<DC/DC type - see note above> --with-max-devices=<yourNumberOfDevicesToBeSupported> --with-cflags=<yourOptions,e.g."-I...",etc> --prefix=<your path to install the driver and shared doc files>
```

For ARM platforms you have to additionally specify the ARCH symbol which for
Linux-based systems 32-bit variant is `ARCH=arm` and for 64-bit is `ARCH=arm64`.
The configure command should look similar to this:

* ARM 64-bit sample (cross-compilation):

  ```bash
  ../configure --with-kernel-incl=<your path to linux kernel include directory> --target=aarch64-openwrt-linux --host=aarch64-openwrt-linux --build=x86_64-pc-linux-gnu ARCH=arm64 --enable-trace --enable-debug --enable-proc --enable-cid --enable-dcdc-hw=<DC/DC type - see note above> --with-max-devices=<yourNumberOfDevicesToBeSupported> --with-cflags=<yourOptions,e.g."-I...",etc> --prefix=<your path to install the driver and shared doc files>
  ```

* ARM 32-bit sample (cross-compilation):

  ```bash
  ../configure --with-kernel-incl=<your path to linux kernel include directory> --target=arm-openwrt-linux --host=arm-openwrt-linux --build=x86_64-pc-linux-gnu ARCH=arm --enable-trace --enable-debug --enable-proc --enable-cid --enable-dcdc-hw=<DC/DC type - see note above> --with-max-devices=<yourNumberOfDevicesToBeSupported> --with-cflags=<yourOptions,e.g."-I...",etc> --prefix=<your path to install the driver and shared doc files>
  ```

### Tips

* If your toolchain binaries are not located under standard PATH directories
you have to add it before running the configure script.  
For example:

   ```bash
   export PATH=<path to ARM toolchain/bin>:$PATH
   ```

### Build TAPI

Run the following command to build the driver. Output files you will find in the directory given
in the `--prefix` parameter to the configure script.

`make clean install`

### Strip your binary

Stripping of the device driver binary is implicitly done on `make install`. If you
want to strip the binary manually, please make sure not to use different options
than `--strip-debug` otherwise the binary might be unusable.
