# Test Application

The test application serves as a functional test for some of the features of
the microkernel and the [user space loader](../loader/). The `qemu-check`
target in the top-level makefile runs this test application in QEMU and then
runs a [check script](../../devel/qemu/check-log.sh) on its output log to
ensure the execution went as expected.
