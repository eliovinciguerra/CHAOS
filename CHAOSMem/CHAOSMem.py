from m5.params import *
from m5.SimObject import SimObject

class CHAOSMem(SimObject):
    type = 'CHAOSMem'
    cxx_class = 'gem5::CHAOSMem'
    cxx_header = "mem/CHAOSMem/CHAOSMem.hh"

    mem = Param.AbstractMemory(NULL, "Main memory pointer.")
    probability = Param.Float(0.0, "Probability (between 0 and 1) of processing cache fault injection")
    bitsToChange = Param.Int(-1, "Number of bits to change in the target cache packet during fault injection (from 0 to 8)")
    firstClock = Param.UInt64(0, "Clock cycle after which the cache fault injector is enabled (default 0)")
    lastClock = Param.UInt64(0, "Clock cycle after which the cache fault injector is disabled (default last clock cycle)")
    faultType = Param.String("random", "Type of alteration to be performed")
    faultMask = Param.String("0", "Bit mask to be applied to the target cache packet value")
    tickToClockRatio = Param.Int(1000, "Ratio between tick and clock cycle (tick/cycle)")
    bitFlipProb = Param.Float(0.9, "Probability (between 0 and 1) of injecting a bit flip fault on 'random' fault type")
    stuckAtZeroProb = Param.Float(0.05, "Probability (between 0 and 1) of injecting a stuck-at-zero fault on 'random' fault type")
    stuckAtOneProb = Param.Float(0.05, "Probability (between 0 and 1) of injecting a stuck-at-one flip fault on 'random' fault type")
    cyclesPermamentFaultCheck = Param.Int(1, "Number of cycles between each periodic check for permanent faults.")
    addr_start = Param.Addr(0, "Start address of the memory-mapped range (default: 0)")
    addr_end = Param.Addr(0, "End address of the memory-mapped range (default: 0, full memory length)")
    writeLog = Param.Bool(True, "Write a log file")