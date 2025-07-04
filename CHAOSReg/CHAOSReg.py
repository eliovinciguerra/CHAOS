from m5.params import *
from m5.SimObject import SimObject

class CHAOSReg(SimObject):
    type = 'CHAOSReg'
    cxx_class = 'gem5::CHAOSReg'
    cxx_header = "CHAOSReg/CHAOSReg.hh"

    probability = Param.Float(0.0, "Probability (between 0 and 1) of injecting faults")
    numBitsToChange = Param.Int(32, "Number of bits to change during fault injection")
    firstClock = Param.UInt64(0, "Clock cycle after which fault injection starts")
    lastClock = Param.UInt64(0, "Clock cycle after which fault injection stops")
    faultType = Param.String("random", "Fault type: bit_flip, stuck_at_zero, stuck_at_one")
    faultMask = Param.String("0", "Bit mask for the fault (optional)")
    regTargetClass = Param.String("both", "Target register class: integer, floating_point, or both")
    PCTarget = Param.Addr(0, "Specific PC value that triggers fault injection")
    cpu = Param.BaseCPU(NULL, "Target CPU")