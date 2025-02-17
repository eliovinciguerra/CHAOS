from m5.params import *
from m5.SimObject import SimObject

class CHAOSMem(SimObject):
    type = 'CHAOSMem'
    cxx_class = 'gem5::CHAOSMem'
    cxx_header = "mem/CHAOSMem/CHAOSMem.hh"

    mem = Param.AbstractMemory(NULL, "Main memory pointer.")
    probability = Param.Float(0.0, "Probability (between 0 and 1) of processing cache fault injection")
    bitsToChange = Param.Int(1, "Number of bits to change in the target cache packet during fault injection (from 0 to 8)")
    firstClock = Param.Int(0, "Clock cycle after which the cache fault injector is enabled (default 0)")
    lastClock = Param.Int(-1, "Clock cycle after which the cache fault injector is disabled (default last clock cycle)")
    tickToClockRatio = Param.Int(1000, "Ratio between tick and clock cycle (tick/cycle)")
    faultMask = Param.String("0", "Bit mask to be applied to the target cache packet value")