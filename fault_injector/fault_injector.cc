#include "fault_injector/fault_injector.hh"
#include "params/FaultInjector.hh"

#include <cassert>

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <bitset>
#include <unordered_map>
#include <functional>
#include <string>     

#include "sim/system.hh"
#include "sim/cur_tick.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst.hh" 
#include "cpu/thread_context.hh"
#include "arch/generic/isa.hh"


namespace gem5
{
/**
 * Costruttore della classe FaultInjector.
 * Inizializza i parametri passati dal file di configurazione e configura l'ambiente per l'iniezione di fault.
 * Viene verificata la validità della CPU e si apre un file di log per tracciare i fault iniettati.
 */
FaultInjector::FaultInjector(const FaultInjectorParams &params)
    : SimObject(params),
      probability(params.probability),
      numBitsToChange(params.numBitsToChange),
      firstClock(params.firstClock),
      lastClock(params.lastClock),
      faultType(params.faultType),
      faultMask(std::bitset<32>(params.faultMask)),
      instTarget(params.instTarget),
      regTargetClass(params.regTargetClass),
      PCTarget(params.PCTarget),
      o3cpu(dynamic_cast<o3::CPU *>(params.o3cpu)),
      tickEvent([this] { this->tick(); }, name())
{
    // Verifica che il puntatore alla CPU sia valido
    if (!o3cpu) {
        throw std::runtime_error("FaultInjector: Invalid CPU pointer");
    }

    // Apre il file di log per salvare i dettagli riguardo i fault iniettati
    logFile.open("fault_injections.log", std::ios::out);
    if (!logFile.is_open()) {
        throw std::runtime_error("FaultInjector: Could not open log file for writing");
    }

    // Inizializza il generatore di numeri casuali
    auto seed = rd(); // TODO - si potrebbe mettere seed tra i parametri per riproducibilità
    rng.seed(seed);
    inter_fault_cycles_dist = std::geometric_distribution<unsigned>(probability);

    // Pianifica primo fault
    unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
    scheduleTickEvent(Cycles(next_fault_cycle_distance)); 
}

/**
 * Metodo statico per creare un FaultInjector con i parametri specificati.
 */
FaultInjector *FaultInjector::create(const FaultInjectorParams &params) 
{
    return new FaultInjector(params);
}

gem5::FaultInjector *
gem5::FaultInjectorParams::create() const
{
    return new gem5::FaultInjector(*this);
}

/**
 * Metodo del distruttore: chiude il file di log.
 */
FaultInjector::~FaultInjector()
{
    if (logFile.is_open()) {
        logFile.close();
    }
}

/**
 * Pianifica l'evento di tick a partire da un certo ritardo in cicli di clock.
 */
void FaultInjector::scheduleTickEvent(Cycles delay)
{
    if (!tickEvent.scheduled())
        schedule(tickEvent, o3cpu->clockEdge(delay));
}

/**
 * Annulla l'evento di tick se è stato pianificato.
 */
void FaultInjector::unscheduleTickEvent()
{
    if (tickEvent.scheduled())
        tickEvent.squash();
}

/**
 * Genera una maschera casuale per modificare un certo numero di bit specificato.
 * @param gen Generatore di numeri casuali.
 * @param numBits Numero di bit da modificare.
 * @return Maschera generata.
 */
int FaultInjector::generateRandomMask(std::mt19937 &gen, int numBits)
{
    int mask = 0;
    std::uniform_int_distribution<int> bitDist(0, 31);

    while (numBits--) {
        mask |= (1 << bitDist(gen));
    }
    return mask;
}

/**
 * Applica un fault a un registro casuale del thread.
 * Classe di registri, tipologia di fault applicato e maschera di bit applicata al valore di registro vengono passati via configurazione
 * Se non specificati, verranno scelti casualmente
 * @param tid ID del thread target.
 */
void FaultInjector::processFault(ThreadID tid)
{
    // Ottiene il contesto del thread con codice tid
    auto *threadContext = o3cpu->tcBase(tid);
    if (!threadContext)
        return;

    // Ottiene il puntatore all'ISA, per poter successivamente accedere ai registri
    BaseISA *isa = threadContext->getIsaPtr();
    if (!isa)
        return;

    // Ottiene le classi di registri disponibili per quell'ISA
    const auto &regClasses = isa->regClasses();
    const RegClass *regClass = nullptr;

    // Ottiene la classe di registri richiesta dal parametro di configurazione
    if (regTargetClass == "both") { // Viene scelta casualmente una delle due classi disponibili
        regClass = (rand() % 2 == 0) ? regClasses[IntRegClass] : regClasses[FloatRegClass];
    } else if (regTargetClass == "integer") {
        regClass = regClasses[IntRegClass];
    } else if (regTargetClass == "floating_point") {
        regClass = regClasses[FloatRegClass];
    }

    if (!regClass || regClass->numRegs() == 0)
        return;

    // Viene scelto casualmente un registro appartenente alla classe di registri ottenuta prima
    std::mt19937 gen(std::random_device{}());
    int randomReg = std::uniform_int_distribution<>(0, regClass->numRegs() - 1)(gen);

    // Se non presente tra i parametri di configurazione, viene generata una maschera di bit casuale
    int mask = faultMask.any() ? faultMask.to_ulong() : generateRandomMask(gen, numBitsToChange);

    RegId regId(*regClass, randomReg);

    try {
        // Prova ad accedere al registro e ne legge il valore
        RegVal regVal = threadContext->getReg(regId);
 
        // Determina il tipo di fault da applicare al valore ottenuto
        std::string chosenFaultType = faultType;
        if (faultType == "random") { // Scelta casuale del tipo di fault
            static std::mt19937 gen(std::random_device{}());
            static std::uniform_int_distribution<int> dis(0, 2);

            // Mappa casualmente il numero a un tipo di fault
            const char* faultTypes[] = {"bit_flip", "stuck_at_zero", "stuck_at_one"};
            chosenFaultType = faultTypes[dis(gen)];
        }
        // Applica il fault al valore del registro
        if (chosenFaultType == "stuck_at_zero") {
            regVal &= ~mask;
        } else if (chosenFaultType == "stuck_at_one") {
            regVal |= mask;
        } else if (chosenFaultType == "bit_flip") {
            regVal ^= mask;
        }

        // Aggiorna il registro con il nuovo valore modificato ottenuto 
        threadContext->setReg(regId, regVal);

        // Registra le operazioni appena eseguite nel file di log
        logFile << "Cycle: " << o3cpu->curCycle()
        << ", Register " << regClass->name() 
        << ": " << randomReg
        << ", Mask: " << std::bitset<32>(mask)
        << ", FaultType: " << chosenFaultType
        << std::dec << std::endl;

        logFile.flush();
    } catch (const std::exception &e) {
        // Cattura l'errore e lo inserisce nel log
        logFile << "Error: Exception caught during fault injection. "
                << "ThreadID: " << tid
                << ", Register: " << randomReg
                << ", Error: " << e.what() << std::endl;

        logFile.flush();
    } catch (...) {
        // Cattura errori generici
        logFile << "Error: Unknown exception during fault injection. "
                << "ThreadID: " << tid
                << ", Register: " << randomReg << std::endl;

        logFile.flush();
    }
}

/**
 * Ad ogni tick controlla se le condizioni indicate per abilitare la fault injection sono verificate
 */
void FaultInjector::tick()
{
    // Se la probabilità di iniezione è nulla, arresta il fault injector
    if (!probability) {
        return;
    }

    // Verifica lo stato dei thread della CPU
    bool allThreadsHalted = true;
    for (ThreadID tid = 0; tid < o3cpu->numThreads; ++tid) {
        auto *threadContext = o3cpu->tcBase(tid);
        if (!threadContext) {
            std::cerr << "ThreadContext non trovato per tid " << tid << std::endl;
            continue;
        }

        // Controlla lo stato del thread
        if (threadContext->status() != ThreadContext::Halted) {
            allThreadsHalted = false;
        }
    }

    // Verifica lo stato della CPU
    bool cpuDrained = (o3cpu->drainState() == DrainState::Drained);

    // Se CPU e thread sono terminati, o se si è superato il limite di cicli di clock impostati, arresta il fault injector
    if (allThreadsHalted || cpuDrained || (lastClock == -1 && o3cpu->curCycle() > lastClock)) {
        unscheduleTickEvent();
        return;
    }

    // Pianifica la chiamata alla funzione per il prossimo fault
    unsigned next_fault_cycle_distance = inter_fault_cycles_dist(rng);
    scheduleTickEvent(Cycles(next_fault_cycle_distance));

    // Verifica che ci siano istruzioni valide decodificate e che si è superato il valore minimo di clock per attivare il fault injector
    if (o3cpu->curCycle() < firstClock || o3cpu->instList.empty()) {
        return;
    }

    // Controlla le condizioni per l'iniezione di fault
    if (checkInst()) {  // Se l'istruzione o il PC corrente corrispondono a quelli indicati
        // Abilita la fault injection in questo ciclo per tutti i thread
        for (ThreadID tid = 0; tid < o3cpu->numThreads; ++tid) {
            processFault(tid);
        }
    }
}


/**
 * Metodo per verificare l'istruzione corrente e il PC corrente
 * Ritorna true se l'istruzione corrente o il PC corrente corrispondono al target, false altrimenti
 */
bool FaultInjector::checkInst()
{
    // Ottiene l'ultima istruzione decodificata
    auto inst = o3cpu->instList.back();
    auto staticInst = inst->staticInst;

    if (!staticInst) {
        return false;
    }

    // Se il PC è uguale al target, viene abilitata la fault injection
    if (PCTarget > 0 && PCTarget == inst->pcState().instAddr()) {
        return true;
    }

    // Definisce una mappa che collega i possibili valori del parametro `instTarget` alle relative caratteristiche dell'istruzione corrente
    static const std::unordered_map<std::string, std::function<bool()>> instCheckMap = {
        {"all", []() { return true; }},
        {"nop", [&]() { return staticInst->isNop(); }},
        {"load", [&]() { return staticInst->isLoad(); }},
        {"store", [&]() { return staticInst->isStore(); }},
        {"atomic", [&]() { return staticInst->isAtomic(); }},
        {"store_conditional", [&]() { return staticInst->isStoreConditional(); }},
        {"inst_prefetch", [&]() { return staticInst->isInstPrefetch(); }},
        {"data_prefetch", [&]() { return staticInst->isDataPrefetch(); }},
        {"integer", [&]() { return staticInst->isInteger(); }},
        {"floating", [&]() { return staticInst->isFloating(); }},
        {"vector", [&]() { return staticInst->isVector(); }},
        {"control", [&]() { return staticInst->isControl(); }},
        {"call", [&]() { return staticInst->isCall(); }},
        {"return", [&]() { return staticInst->isReturn(); }},
        {"direct_ctrl", [&]() { return staticInst->isDirectCtrl(); }},
        {"indirect_ctrl", [&]() { return staticInst->isIndirectCtrl(); }},
        {"cond_ctrl", [&]() { return staticInst->isCondCtrl(); }},
        {"uncond_ctrl", [&]() { return staticInst->isUncondCtrl(); }},
        {"serializing", [&]() { return staticInst->isSerializing(); }},
        {"serialize_before", [&]() { return staticInst->isSerializeBefore(); }},
        {"serialize_after", [&]() { return staticInst->isSerializeAfter(); }},
        {"squash_after", [&]() { return staticInst->isSquashAfter(); }},
        {"full_mem_barrier", [&]() { return staticInst->isFullMemBarrier(); }},
        {"read_barrier", [&]() { return staticInst->isReadBarrier(); }},
        {"write_barrier", [&]() { return staticInst->isWriteBarrier(); }},
        {"non_speculative", [&]() { return staticInst->isNonSpeculative(); }},
        {"unverifiable", [&]() { return staticInst->isUnverifiable(); }},
        {"syscall", [&]() { return staticInst->isSyscall(); }},
        {"macroop", [&]() { return staticInst->isMacroop(); }},
        {"microop", [&]() { return staticInst->isMicroop(); }},
        {"delayed_commit", [&]() { return staticInst->isDelayedCommit(); }},
        {"last_microop", [&]() { return staticInst->isLastMicroop(); }},
        {"first_microop", [&]() { return staticInst->isFirstMicroop(); }},
        {"htm_start", [&]() { return staticInst->isHtmStart(); }},
        {"htm_stop", [&]() { return staticInst->isHtmStop(); }},
        {"htm_cancel", [&]() { return staticInst->isHtmCancel(); }},
        {"htm_cmd", [&]() { return staticInst->isHtmCmd(); }}
    };

    // Viene cercata l'istruzione corrente in `instCheckMap`. Se viene trovata, viene abilitata la fault injection
    auto it = instCheckMap.find(instTarget);
    if (it != instCheckMap.end()) {
        return it->second(); // Esegue la funzione associata
    }

    return false; // Restituisce false se `instTarget` non è nella mappa
}

} // namespace gem5
