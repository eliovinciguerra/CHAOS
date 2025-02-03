from m5.params import *
from m5.SimObject import SimObject

# Definizione della classe FaultInjector che estende SimObject
class FaultInjector(SimObject):
    # Tipo specifico di SimObject
    type = 'FaultInjector'

    # Associazione alla classe C++ e al file header corrispondente
    cxx_class = 'gem5::FaultInjector'  # Nome della classe C++ implementata
    cxx_header = "fault_injector/fault_injector.hh"  # Header file contenente la definizione della classe

    # Parametri configurabili dell'iniettore di fault
    probability = Param.Float(
        0.0, 
        "Probability (between 0 and 1) of processing fault injection"
    )  # Probabilità con cui viene iniettato un fault (valore tra 0 e 1)

    numBitsToChange = Param.Int(
        32, 
        "Number of bits to change in the target register during fault injection"
    )  # Numero di bit che saranno alterati durante l'iniezione del fault

    firstClock = Param.Int(
        0, 
        "Clock cycle after which the fault injector is enabled (default 0)"
    )  # Ciclo di clock a partire dal quale l'iniettore di fault è attivo

    lastClock = Param.Int(
        -1, 
        "Clock cycle after which the fault injector is disabled (default last clock cycle)"
    )  # Ciclo di clock oltre il quale l'iniettore di fault è disabilitato (-1 significa fino alla fine)

    faultType = Param.String(
        "random", 
        "Type of alteration to be performed to the target register value"
    )  # Tipo di fault da applicare: es. inversione di bit ("bit_flip")

    faultMask = Param.String(
        "0", 
        "Bit mask to be applied to the target register value"
    )  # Maschera di bit da applicare al valore del registro bersaglio

    instTarget = Param.String(
        "all", 
        "Type of instruction that triggers fault injection"
    )  # Tipo di istruzione che attiva l'iniezione di fault (es. "load", "store", "all")

    regTargetClass = Param.String(
        "both", 
        "Type of registers to be injected (default integer registers)"
    )  # Tipo di registri bersaglio: "integer", "floating_point", "both" (default)

    PCTarget = Param.Addr(
        0, 
        "PC value that triggers fault injection"
    )  # Valore del Program Counter (PC) che attiva l'iniezione di fault

    o3cpu = Param.BaseCPU(
        NULL, 
        "CPU to inject faults into"
    )  # CPU O3 bersaglio per l'iniezione di fault
