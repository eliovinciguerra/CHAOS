
# CHAOS – Controlled Hardware fAult injectOr System for gem5

CHAOS is a fault injector for gem5, and its distinctive feature lies in its modular and open-source nature.

CHAOS is organized into 3 different modules, which offer the ability to inject faults into:
- CPU architectural registers (CHAOSReg).
- Cache lines (CHAOSCache).
- Main memory locations (CHAOSMem).

All ISAs (ARM, NULL, MIPS, POWER, RISCV, SPARC, X86) and CPU models (O3CPU, TimingSimpleCPU, MinorCPU, AtomicSimpleCPU, DerivO3CPU, SimpleCPU) supported by gem5 are fully compatible with CHAOS.

It is also important to note that CHAOS has been implemented as a SimObject, which makes it highly customizable and easy to install, even when upgrading to a new version of gem5.

## Types of Faults

Each module supports three distinct types of faults that modify the contents of its target component (architectural registers, cache lines, or memory locations):
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
- *faultMask*: A 32 bit integer representing a bitmask to be applied to the target register. If set to 0, a random bitmask is generated.
- *bitsToChange*: If *faultMask* is set to 0, this integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *regTargetClass*: A string indicating the class of architectural registers that can be targeted. Options include:
    - 'integer' – integer registers.
    - 'floating_point' – floating-point registers.
    - 'both' – randomly selects between integer and floating-point registers.
- *bitFlipProb*: if *faultType* is 'bit_flip', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'bit_flip' fault type.
- *stuckAtZeroProb*: if *faultType* is 'stuck_at_zero', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_zero' fault type
- *stuckAtOneProb*: if *faultType* is 'stuck_at_one', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_one' fault type
- *cyclesPermamentFaultCheck*: Number of cycles between each periodic check for permanent faults.
- *PCTarget*: A numerical value specifying the program counter (PC) address at which CHAOS should be activated.
- *writeLog*: Write a log file of the injected faults.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: 0.
- *regTargetClass*: 'both'.
- *bitFlipProb*: 0.9.
- *stuckAtZeroProb*: 0.05.
- *stuckAtOneProb*: 0.05.
- *cyclesPermamentFaultCheck*: 1.
- *PCTarget*: 0 (by default, no faults are injected based on the PC value).
- *writeLog*: True

The only parameter that lacks a predefined default value is *bitsToChange*. If required but unspecified by the user, a random value will be dynamically assigned using the *std::mt19937* random number generator.

After the simulation run, a log file named *fault_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Cycle*: the clock cycle in which the fault is injected.
- *Register*: the identifier of the target register.
- *Mask*: the applied mask.
- *FaultType*: the type of fault injected.

The *stats.txt* file automatically generated by gem5 will also report several aggregate metrics, including:
- *system.CHAOSReg.numFaultsInjected*: Total number of faults injected.
- *system.CHAOSReg.numBitFlips*: Number of bit flip faults injected.
- *system.CHAOSReg.numStuckAtZero*: Number of stuck-at-0 faults injected.
- *system.CHAOSReg.numStuckAtOne*: Number of stuck-at-1 faults injected.
- *system.CHAOSReg.numPermanentFaults*: Total number of permanent faults injected.

## Usage of CHAOSCache

CHAOSCache can be configured to inject specific faults with cycle-level precision.

The following parameters are configurable:
- *targetCache*: Pointer to the faulty cache.
- *probability*: A floating-point value between 0 and 1 that specifies the probability threshold for activating CHAOS in a given clock cycle.
- *firstClock*: An integer value indicating the first clock cycle in which CHAOS can be triggered.
- *lastClock*: An integer value specifying the last permissible clock cycle for fault injection.
- *faultType*: A string specifying the type of fault to be injected. Available options include:
    - 'bit_flip' – a single-bit inversion.
    - 'stuck_at_zero' – forcing a bit to remain at logic level 0.
    - 'stuck_at_one' – forcing a bit to remain at logic level 1.
    - 'random' – randomly selects one of the above fault types.
- *faultMask*: A byte representing a bitmask to be applied to the target (from '0' to '255'). If set to '0', a random bitmask is generated.
- *bitsToChange*: This integer parameter determines the number of bits to be affected by the randomly generated bitmask.
- *corruptionSize*: This integer parameter determines the number of bytes to be affected in each fault injection iteration.
- *tickToClockRatio*: The ratio between gem5 ticks and clock cycle.
- *bitFlipProb*: if *faultType* is 'bit_flip', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'bit_flip' fault type.
- *stuckAtZeroProb*: if *faultType* is 'stuck_at_zero', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_zero' fault type
- *stuckAtOneProb*: if *faultType* is 'stuck_at_one', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_one' fault type
- *cyclesPermamentFaultCheck*: Number of cycles between each periodic check for permanent faults.
- *writeLog*: Write a log file of the injected faults.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: '0'.
- *corruptionSize*: 1.
- *tickToClockRatio*: 1000.
- *bitFlipProb*: 0.9.
- *stuckAtZeroProb*: 0.05
- *stuckAtOneProb*: 0.05
- *cyclesPermamentFaultCheck*: 1.
- *writeLog*: True.

The only parameter that lacks a predefined default value is *bitsToChange*. If required but unspecified by the user, a random value will be dynamically assigned using the *std::mt19937* random number generator.

After the simulation run, a log file named *cache_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Tick*: the tick in which the fault is injected.
- *Cache Block Addr*: the cache block address in which the fault is injected.
- *Byte offset*: the byte offset of che cache block in which the fault is injected.
- *Mask*: the applied mask in decimal depresentation.
- *Fault Type*: type of the injected fault.

The *stats.txt* file automatically generated by gem5 will also report several aggregate metrics, including:
- *system.CHAOSCache.numFaultsInjected*: Total number of faults injected.
- *system.CHAOSCache.numBitFlips*: Number of bit flip faults injected.
- *system.CHAOSCache.numStuckAtZero*: Number of stuck-at-0 faults injected.
- *system.CHAOSCache.numStuckAtOne*: Number of stuck-at-1 faults injected.
- *system.CHAOSCache.numPermanentFaults*: Total number of permanent faults injected.

## Usage of CHAOSMem

CHAOSMem can be configured to inject specific faults with clock cycle-level granularity.

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
- *bitFlipProb*: if *faultType* is 'bit_flip', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'bit_flip' fault type.
- *stuckAtZeroProb*: if *faultType* is 'stuck_at_zero', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_zero' fault type
- *stuckAtOneProb*: if *faultType* is 'stuck_at_one', this floating point parameter determines the probability (between 0 and 1) of injecting a bit flip fault on 'stuck_at_one' fault type
- *cyclesPermamentFaultCheck*: Number of cycles between each periodic check for permanent faults.
- *addr_start*: Start address, specifies the starting address of CHAOSMem.
- *addr_end*: End address, specifies the last valid address usable by CHAOSMem.
- *writeLog*: Write a log file of the injected faults.

Each parameter is assigned a default value as follows:
- *probability*: 0.0.
- *firstClock*: 0.
- *lastClock*: -1.
- *faultType*: 'random'.
- *faultMask*: '0'.
- *tickToClockRatio*: 1000.
- *bitFlipProb*: 0.9.
- *stuckAtZeroProb*: 0.05
- *stuckAtOneProb*: 0.05
- *cyclesPermamentFaultCheck*: 1.
- *addr_start*: 0.
- *addr_end*: 0, full memory length.
- *writeLog*: True.

The only parameter that lacks a predefined default value is *bitsToChange*. If required but unspecified by the user, a random value will be dynamically assigned using the *std::mt19937* random number generator.

After the simulation run, a log file named *main_mem_injections.log* will be generated. Each line in the file will record an injected fault, containing the following details:
- *Tick*: the tick in which the fault is injected.
- *target addr*: the memory target address in which the fault is injected.
- *Mask*: the applied mask.

The *stats.txt* file automatically generated by gem5 will also report several aggregate metrics, including:
- *system.CHAOSMem.numFaultsInjected*: Total number of faults injected.
- *system.CHAOSMem.numBitFlips*: Number of bit flip faults injected.
- *system.CHAOSMem.numStuckAtZero*: Number of stuck-at-0 faults injected.
- *system.CHAOSMem.numStuckAtOne*: Number of stuck-at-1 faults injected.
- *system.CHAOSMem.numPermanentFaults*: Total number of permanent faults injected.


## Examples of CHAOSReg

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/two_level.py*

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
fault_injector = CHAOSReg(
    cpu = system.cpu,
    probability = probability
)
system.CHAOSReg = fault_injector
```

Now you can run gem5 without any further modifications.

In the */CHAOS/examples* directory, you can find *two_level.py*, which has already been modified.

## Examples of CHAOSCache

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/two_level.py*

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
fault_injector = CHAOSCache(
    target_cache=system.cpu.dcache,
    probability = probability
)
system.CHAOSCache = fault_injector
```

Now you can run gem5 without any further modifications.

## Examples of CHAOSMem

For testing purposes, modify the example file provided by gem5: */path/to/gem5/configs/learning_gem5/part1/two_level.py*

Before the definition of *root*, add the following in order to test the tool using the default configuration:

```python
fault_injector = CHAOSMem(
    mem=system.mem_ctrl.dram,
    probability = probability
)
system.CHAOSMem = fault_injector
```

Now you can run gem5 without any further modifications.

In the */CHAOS/examples* directory, you can find *two_level.py*, which has already been modified.

## Authors

- [@eliovinciguerra](https://www.github.com/eliovinciguerra)
- [@Haimrich](https://www.github.com/Haimrich)
- [@mpalesi](https://github.com/mpalesi)
- [@DanieleZinghirino](https://github.com/DanieleZinghirino)