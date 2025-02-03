
# CHAOS – Controlled Hardware fAult injectOr System for gem5

CHAOS is a fault injector for gem5, and its distinctive feature lies in its modular and open-source nature.

At the current stage of the project, all ISAs supported by gem5 are compatible. However, the only supported CPU model is the Out-of-Order (O3) core.

It is also important to note that CHAOS has been implemented as a SimObject, which makes it highly customizable and easy to install, even when upgrading to a new version of gem5.
## Types of Faults

CHAOS currently supports three distinct types of faults, enabling modifications to the contents of target registers:
1. Bit flip: This fault type induces a bit-flipping operation on the register contents, where one or more bits transition from 0 to 1 or vice versa. This alteration models transient faults in the system, which may arise due to physical phenomena such as radiation, electromagnetic interference, hardware malfunctions, or material defects in the circuitry.
2. Stuck at zero: In this configuration, one or more bits within the register are permanently forced to 0. This simulates permanent faults in the system, akin to a node or signal line being shorted to ground.
3. Stuck at one: Similar to the previous configuration, this fault type forces one or more bits in the register to remain at 1, simulating permanent errors in the system, such as a node being tied to the power supply.
## Installation

Use the Makefile to clone the RISC-V toolchain (used for testing) and gem5, move the fault injector into the gem5 directory, and compile everything.

To use it, run:

```bash
  make
```
    
## Usage

CHAOS can be configured to inject specific types of faults with clock cycle-level granularity.

The following parameters are configurable:
- *o3cpu*: Defines the CPU model used by gem5.
- *probability*: A floating-point value between 0 and 1 that specifies the probability threshold for activating the fault injector in a given clock cycle.
- *firstClock*: An integer value indicating the first clock cycle in which the fault injector can be triggered.
- *lastClock*: An integer value specifying the last permissible clock cycle for fault injection.
- *faultType*: A string specifying the type of fault to be injected. Available options include:
    - 'bit_flip' – a single-bit inversion.
    - 'stuck_at_zero' – forcing a bit to remain at logic level 0.
    - 'stuck_at_one' – forcing a bit to remain at logic level 1.
    - 'random' – randomly selects one of the above fault types.
- *faultMask*: A string representing a bitmask to be applied to the target register. If set to '0', a random bitmask is generated.
- *numBitsToChange*: If *faultMask* is set to '0', this integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *instTarget*: A string specifying the instruction types that the fault injector can target. Examples include:
    - 'nop' – no-operation instructions.
    - 'load' – memory load operations.
    - 'store' – memory store operations.
    - 'integer' – integer arithmetic instructions.
    - floating_point' – floating-point arithmetic operations.
    - 'return' – function return operations.
    - 'all' – allows fault injection across all instruction types.
- *regTargetClass*: A string indicating the class of registers that can be targeted. Options include:
    - 'integer' – integer registers.
    - 'floating_point' – floating-point registers.
    - 'both' – randomly selects between integer and floating-point registers.
- *PCTarget*: A numerical value specifying the program counter (PC) address at which the fault injector should be activated.

Notably, the *instTarget* parameter supports all instruction types defined in *gem5/src/cpu/static_inst.hh*, including but not limited to: 'atomic', 'store_conditional', 'inst_prefetch', 'data_prefetch', 'vector', 'control', 'call', 'direct_ctrl', 'indirect_ctrl', 'full_mem_barrier', 'read_barrier', 'write_barrier', 'serializing', 'serialize_before', 'squash_after', 'syscall', 'macroop', 'microop', 'htm_start', 'htm_stop', 'htm_cancel', and 'htm_cmd'.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: '0'.
- *instTarget*: 'all'.
- *regTargetClass*: 'both'.
- *PCTarget*: 0 (by default, no faults are injected based on the PC value).

The only parameter that lacks a predefined default value is *numBitsToChange*. If required but unspecified by the user, a random value will be dynamically assigned using the *std::mt19937* random number generator.

After the simulation run, a log file named *fault_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Cycle*: the clock cycle in which the fault is injected.
- *Register*: the identifier of the target register.
- *Mask*: the applied mask.
- *FaultType*: the type of fault injected.
## Examples

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/simple-riscv.py.*

Change:
```python
system.cpu = RiscvTimingSimpleCPU()
```
to:
```python
system.cpu = RiscvO3CPU()
```

As at the moment this tool works only with O3 CPUs.

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
fault_injector = FaultInjector(
    o3cpu = system.cpu,
    probability = probability
)
system.fault_injector = fault_injector
```

Now you can run gem5 without any further modifications.
## Authors

- [@eliovinciguerra](https://www.github.com/eliovinciguerra)
- [@DanieleZinghirino](https://github.com/DanieleZinghirino)
- [@Haimrich](https://www.github.com/Haimrich)
- [@mpalesi](https://github.com/mpalesi)

