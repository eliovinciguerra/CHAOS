from m5.params import *
from m5.SimObject import SimObject

class CHAOSReg(SimObject):
    type = 'CHAOSReg'

    cxx_class = 'gem5::CHAOSReg'
    cxx_header = "CHAOSReg/CHAOSReg.hh"

    # CHAOSReg parameters
    probability = Param.Float(
        0.0, 
        "Probability (between 0 and 1) of processing fault injection"
    )

    numBitsToChange = Param.Int(
        32, 
        "Number of bits to change in the target register during fault injection"
    )

    firstClock = Param.Int(
        0, 
        "Clock cycle after which the fault injector is enabled (default 0)"
    )

    lastClock = Param.Int(
        -1, 
        "Clock cycle after which the fault injector is disabled (default last clock cycle)"
    )

    faultType = Param.String(
        "random", 
        "Type of alteration to be performed to the target register value"
    )

    faultMask = Param.String(
        "0", 
        "Bit mask to be applied to the target register value"
    )

    instTarget = Param.String(
        "all", 
        "Type of instruction that triggers fault injection"
    )

    regTargetClass = Param.String(
        "both", 
        "Type of registers to be injected (default integer registers)"
    )

    PCTarget = Param.Addr(
        0, 
        "PC value that triggers fault injection"
    )

    cpu = Param.BaseCPU(
        NULL, 
        "CPU to inject faults into"
    )
