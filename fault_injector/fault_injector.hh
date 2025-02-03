#ifndef __FAULT_INJECTOR_HH__
#define __FAULT_INJECTOR_HH__

#include "sim/sim_object.hh"
#include "params/FaultInjector.hh"
#include "cpu/o3/cpu.hh"
#include <iostream>
#include <random>
#include <bitset>
#include "sim/cur_tick.hh"
#include "sim/eventq.hh"
#include <fstream>

namespace gem5
{
    /**
     * FaultInjector è una classe che si occupa di iniettare fault nei registri
     * della CPU simulata. I fault sono iniettati con una probabilità configurabile,
     * permettendo di testare la resilienza della CPU in presenza di errori hardware.
     * L'iniezione dei fault viene eseguita su registri a livello di thread, con
     * un ciclo di iniezione che avviene ogni ciclo di clock.
     */

    class FaultInjector : public SimObject
    {
     private:
        o3::CPU *o3cpu;               // Puntatore alla CPU O3 (out-of-order)

        float probability;            /* Probabilità di iniettare un fault in un determinato ciclo
                                         Valori consentiti --> float compresi tra 0 e 1
                                         Valore di default --> 0 */

        int numBitsToChange;          /* Numero di bit da modificare per iniettare il fault
                                         Se "faultMask" è diverso da "0", questo parametro è ininfluente nella simulazione
                                         Valori consentiti --> interi compresi tra 1 e 32
                                         Valore di default --> 32 */

        int firstClock, lastClock;    /* Range di cicli di clock nei quali abilitare la fault injection
                                         Valore di default di firstClock --> 0
                                         Valore di default di lastClock --> ultimo ciclo di simulazione*/

        std::string faultType;        /* Tipologia di alterazione da applicare ai bit del registro target
                                         Valori consentiti --> "bit_flip" , 
                                                               "stuck_at_zero", 
                                                               "stuck_at_one", 
                                                               "random" (sceglie casualmente una metodologia)
                                         Valore di default --> "random"*/

        std::bitset<32> faultMask;    /* Maschera dei bit da applicare ai bit del registro target
                                         Valori consentiti --> una stringa contenente bit compresi tra "0" "11111111111111111111111111111111"
                                         Valore di default --> "0" (in questo caso viene generata una maschera casuale di numBitsToChange bit)*/

        std::string instTarget;       /* Tipologia di istruzione che abilita l'iniezione del fault
                                         Valori consentiti --> "all" (abilita l'iniezione per qualunque istruzione)
                                                                "nop"
                                                                "load"
                                                                "store"
                                                                "atomic"
                                                                "store_conditional"
                                                                "inst_prefetch"
                                                                "data_prefetch"
                                                                "integer"
                                                                "floating"
                                                                "vector"
                                                                "control"
                                                                "call"
                                                                "return"
                                                                "direct_ctrl"
                                                                "indirect_ctrl"
                                                                "serializing"
                                                                "serialize_before"
                                                                "serialize_after"
                                                                "squash_after"
                                                                "full_mem_barrier"
                                                                "read_barrier" 
                                                                "write_barrier" 
                                                                "non_speculative"
                                                                "unverifiable"
                                                                "syscall"
                                                                "macroop"
                                                                "microop"
                                                                "delayed_commit"
                                                                "last_microop"
                                                                "first_microop"
                                                                "htm_start"
                                                                "htm_stop"
                                                                "htm_cancel"
                                                                "htm_cmd"
                                        Valore di default --> "all" */

        std::string regTargetClass;   /* Classe di registri da cui scegliere il target della fault injection
                                         Valori consentiti --> "both" (viene scelta casualmente una classe)
                                                               "integer"
                                                               "floating_point"
                                         Valore di default --> "both" */

        Addr PCTarget;                /* Indirizzo del Program Counter (PC) nel quale abilitare la fault injection
                                         Valore di default --> 0 */

        EventFunctionWrapper tickEvent; // Evento per eseguire la funzione tick() periodicamente
        std::ofstream logFile;         // File di log per tracciare gli eventi

        
         /**
         * Funzione che genera una maschera casuale di bit da usare per l'iniezione del fault.
         * La maschera è utilizzata per cambiare "numBits" bit casuali nel valore del registro.
         * 
         * @param rng Generatore di numeri casuali.
         * @param numBits Numero di bit da modificare (bit della maschera settati a "1").
         * @return La maschera di bit generata. 
         */
        int generateRandomMask(std::mt19937 &gen, int numBits);

        /**
        * Distruttore che chiude il file di log.
        */
        ~FaultInjector();

    public:
        /**
         * Funzione che inietta un fault nei registri di un thread specificato.
         * Seleziona casualmente un registro e inietta il fault modificando i suoi bit.
         * 
         * @param tid Identificatore del thread a cui iniettare il fault.
         */
        FaultInjector(const FaultInjectorParams &p);

        // Funzione statica per creare un oggetto FaultInjector
        static FaultInjector *create(const FaultInjectorParams &params);

         /**
         * Funzione che inietta un fault nei registri di un thread specificato.
         * Seleziona casualmente un registro e inietta il fault modificando i suoi bit.
         * 
         * @param tid Identificatore del thread a cui iniettare il fault.
         */
        void processFault(ThreadID tid);

        /**
         * Pianifica l'iniezione di un fault ogni ciclo di clock.
         */
        void tick();

        // Funzione per pianificare l'iniezione del fault a intervalli di cicli specifici
        void scheduleTickEvent(Cycles delay);

        // Funzione per annullare la pianificazione dell'evento tick
        void unscheduleTickEvent();

        /**
         * Verifica se l'istruzione corrente o il PC corrispondono a quelli target in cui iniettare un fault.
         * 
         * @return true se l'istruzione corrente è quella target o se il PC è quello target, altrimenti false.
         */
        bool checkInst(); 
    };

} // namespace gem5

#endif // __FAULT_INJECTOR_HH__
