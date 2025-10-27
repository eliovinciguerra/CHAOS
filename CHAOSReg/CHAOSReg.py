from m5.params import *
from m5.SimObject import SimObject

class CHAOSReg(SimObject):
    type = 'CHAOSReg'
    cxx_class = 'gem5::CHAOSReg'
    cxx_header = "CHAOSReg/CHAOSReg.hh"

    cpu = Param.BaseCPU(NULL, "Target CPU")
    probability = Param.Float(0.0, "Probability (between 0 and 1) of injecting faults")
    bitsToChange = Param.Int(-1, "Number of bits to change during fault injection")
    firstClock = Param.UInt64(0, "Clock cycle after which fault injection starts")
    lastClock = Param.UInt64(0, "Clock cycle after which fault injection stops")
    faultType = Param.String("random", "Fault type: bit_flip, stuck_at_zero, stuck_at_one")
    faultMask = Param.UInt32(0, "Bit mask for the fault (optional)")
    regTargetClass = Param.String("both", "Target register class: integer, floating_point, or both")
    bitFlipProb = Param.Float(0.9, "Probability (between 0 and 1) of injecting a bit flip fault on 'bit_flip' fault type")
    stuckAtZeroProb = Param.Float(0.05, "Probability (between 0 and 1) of injecting a stuck-at-zero fault on 'stuck_at_zero' fault type")
    stuckAtOneProb = Param.Float(0.05, "Probability (between 0 and 1) of injecting a stuck-at-one flip fault on 'stuck_at_one' fault type")
    cyclesPermamentFaultCheck = Param.Int(1, "Number of cycles between each periodic check for permanent faults.")
    PCTarget = Param.Addr(0, "Specific PC value that triggers fault injection")
    writeLog = Param.Bool(True, "Write a log file")