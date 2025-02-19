
# CHAOS – Controlled Hardware fAult injectOr System for gem5

CHAOS is a fault injector for gem5, and its distinctive feature lies in its modular and open-source nature.

CHAOS is organized into 3 different modules, which offer the ability to inject faults into:
- CPU registers (CHAOSReg).
- Cache memory (CHAOSTags).
- Main memory (CHAOSMem).

At the current stage of the project, all ISAs supported by gem5 are compatible. However, for CHAOSReg the only supported CPU model is the Out-of-Order (O3) core.

It is also important to note that CHAOS has been implemented as a SimObject, which makes it highly customizable and easy to install, even when upgrading to a new version of gem5.
## Types of Faults

CHAOSReg, CHAOSTags and CHAOSMem currently supports three distinct types of faults, enabling modifications to the contents of target registers:
1. Bit flip: This fault type induces a bit-flipping operation on the register contents, where one or more bits transition from 0 to 1 or vice versa. This alteration models transient faults in the system, which may arise due to physical phenomena such as radiation, electromagnetic interference, hardware malfunctions, or material defects in the circuitry.
2. Stuck at zero: In this configuration, one or more bits within the register are permanently forced to 0. This simulates permanent faults in the system, akin to a node or signal line being shorted to ground.
3. Stuck at one: Similar to the previous configuration, this fault type forces one or more bits in the register to remain at 1, simulating permanent errors in the system, such as a node being tied to the power supply.

## Installation

Use the Makefile to clone the RISC-V toolchain (used for testing) and gem5, move the fault injector into the gem5 directory, and compile everything.

To clone gem5 and compile it with CHAOS, run:

```bash
  make all
```

To clone RISC-V toolchain (used later for the examples) and compile it, run:

```bash
  make toolchain
```


    
## Usage of CHAOSReg

CHAOSReg can be configured to inject specific types of faults with clock cycle-level granularity.

The following parameters are configurable:
- *cpu*: Defines the CPU model used by gem5.
- *probability*: A floating-point value between 0 and 1 that specifies the probability threshold for activating CHAOS in a given clock cycle.
- *firstClock*: An integer value indicating the first clock cycle in which CHAOS can be triggered.
- *lastClock*: An integer value specifying the last permissible clock cycle for fault injection.
- *faultType*: A string specifying the type of fault to be injected. Available options include:
    - 'bit_flip' – a single-bit inversion.
    - 'stuck_at_zero' – forcing a bit to remain at logic level 0.
    - 'stuck_at_one' – forcing a bit to remain at logic level 1.
    - 'random' – randomly selects one of the above fault types.
- *faultMask*: A string representing a bitmask to be applied to the target register. If set to '0', a random bitmask is generated.
- *numBitsToChange*: If *faultMask* is set to '0', this integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *instTarget*: A string specifying the instruction types that CHAOS can target. Examples include:
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
- *PCTarget*: A numerical value specifying the program counter (PC) address at which CHAOS should be activated.

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

## Usage of CHAOSTags

CHAOSTags can be configured to inject specific bit flips with cycle-level precision.

CHAOSTags models only the cache tag. The provided simobject sits between the cache and the default BaseTags model, injecting bit flips into packets arriving at the cache. To simplify installation, a complete set of Tags extending BaseTags has been provided, modified to integrate seamlessly with CHAOSTags.

The following parameters are configurable:
- *probability*: A floating-point value between 0 and 1 that specifies the probability threshold for activating CHAOS in a given clock cycle.
- *firstClock*: An integer value indicating the first clock cycle in which CHAOS can be triggered.
- *lastClock*: An integer value specifying the last permissible clock cycle for fault injection.
- *faultType*: A string specifying the type of fault to be injected. Available options include:
    - 'bit_flip' – a single-bit inversion.
    - 'stuck_at_zero' – forcing a bit to remain at logic level 0.
    - 'stuck_at_one' – forcing a bit to remain at logic level 1.
    - 'random' – randomly selects one of the above fault types.
- *faultMask*: A byte representing a bitmask to be applied to the target (from '0' to '255'). If set to '0', a random bitmask is generated.
- *numBitsToChangePerByte*: If *faultMask* is set to '0', this integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *numBytesToChange*: Number of bits to change in the target cache packet during fault injection. (TODO)
- *tickToClockRatio*: The ratio between gem5 ticks and clock cycle.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: '0'.
- *numBitsToChangePerByte*: 1.
- *numBytesToChange*: 1.
- *tickToClockRatio*: 1000.

After the simulation run, a log file named *cache_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Tick*: the tick in which the fault is injected.
- *Packet Byte*: the packet byte in which the fault is injected.
- *Mask*: the applied mask.

## Usage of CHAOSMem

CHAOSMem can be configured to inject specific bit flip with clock cycle-level granularity.

The following parameters are configurable:
- *mem*: Define the target memory.
- *probability*: A floating-point value between 0 and 1 that specifies the probability threshold for activating CHAOS in a given clock cycle.
- *firstClock*: An integer value indicating the first clock cycle in which CHAOS can be triggered.
- *lastClock*: An integer value specifying the last permissible clock cycle for fault injection.
- *faultType*: A string specifying the type of fault to be injected. Available options include:
    - 'bit_flip' – a single-bit inversion.
    - 'stuck_at_zero' – forcing a bit to remain at logic level 0.
    - 'stuck_at_one' – forcing a bit to remain at logic level 1.
    - 'random' – randomly selects one of the above fault types.
- *faultMask*: A byte representing a bitmask to be applied to the target (from '0' to '255'). If set to '0', a random bitmask is generated.
- *bitsToChange*: If *faultMask* is set to '0', this integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *tickToClockRatio*: The ratio between gem5 ticks and clock cycle.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: '0'.
- *bitsToChange*: 1.
- *tickToClockRatio*: 1000.

After the simulation run, a log file named *main_mem_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Tick*: the tick in which the fault is injected.
- *target addr*: the memory target address in which the fault is injected.
- *Mask*: the applied mask.


## Examples of CHAOSReg

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
fault_injector = CHAOSReg(
    cpu = system.cpu,
    probability = probability
)
system.CHAOSReg = fault_injector
```

Now you can run gem5 without any further modifications.

In the */CHAOS/examples* directory, you can find *simple-riscv.py*, which has already been modified.

## Examples of CHAOSTags

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/two_level.py*

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
system.l2cache.tags.probability = probability
```

Now you can run gem5 without any further modifications.

In the */CHAOS/examples* directory, you can find *two_level.py*, which has already been modified.

## Examples of CHAOSMem

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/simple-riscv.py.*

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
fault_injector = CHAOSMem(
    mem=system.mem_ctrl.dram,
    probability = probability
)
system.CHAOSMem = fault_injector
```

Now you can run gem5 without any further modifications.

In the */CHAOS/examples* directory, you can find *simple-riscv.py*, which has already been modified.

## Authors

- [@eliovinciguerra](https://www.github.com/eliovinciguerra)
- [@DanieleZinghirino](https://github.com/DanieleZinghirino)
- [@Haimrich](https://www.github.com/Haimrich)
- [@mpalesi](https://github.com/mpalesi)

