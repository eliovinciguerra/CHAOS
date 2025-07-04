from m5.params import *
from m5.proxy import *
from m5.objects.Cache import Cache

class CHAOSCache(Cache):
    type = 'CHAOSCache' 
    cxx_header = "mem/cache/CHAOSCache/CHAOSCache.hh"
    cxx_class = 'gem5::CHAOSCache'
    
    probability = Param.Float(0.0, "Probability (between 0 and 1) of injecting faults")
    injectOnRead = Param.Bool(True, "Fault Injection on read")
    injectOnWrite = Param.Bool(True, "Fault Injection on write")
    numBitsToChange = Param.Int(-1, "Probability (between 0 and 1) of injecting faults")
    firstClock = Param.UInt64(0, "Clock cycle after which fault injection starts")
    lastClock = Param.UInt64(0, "Clock cycle after which fault injection stops")
    faultType = Param.String("random", "Fault type: bit_flip, stuck_at_zero, stuck_at_one")
    tickToClockRatio = Param.Int(1000, "Ratio between tick and clock cycle (tick/cycle)")