# uCore-SMP
A Symmetric Multiprocessing OS Kernel over RISC-V

## Supported Devices

- HiFive Unmatched (fu740)
- QEMU

## Documentation

[click here](doc/doc.pdf)

## Usage

### Running on HiFive Unmatched (fu740)

This version is only available on xiji gitlab. 

There are some versions that can pass the test, they are tagged as fu740-test-vXXX.

```shell
git clone https://gitlab.eduxiji.net/hlw2014/ucore-smp.git
cd uCore-SMP

# the fu740-test branch exists only on xiji gitlab, not on github
git checkout fu740-test
(or fu740-test-v0.5, fu740-test-v0.4 ...)

# this command will generate a kernel image called 'os.bin' in the root directory of the project, which can be loaded into memory via u-boot.
make all
```

### Running on QEMU

```shell
git clone https://github.com/NKU-EmbeddedSystem/uCore-SMP.git
(or https://gitlab.eduxiji.net/hlw2014/ucore-smp.git)

cd uCore-SMP

# QEMU version is on fu740 branch
git checkout fu740

# make user programs (e.g. shell)
make user

# make kernel
make

# run with QEMU
make run
```
