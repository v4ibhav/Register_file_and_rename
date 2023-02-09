#include "renamer.h"
#include <vector>

using namespace std;

#define foru(i,n) for(uint64_t i = 0; i<n; i++)

renamer::renamer(uint64_t n_log_regs,uint64_t n_phys_regs,uint64_t n_branches,uint64_t n_active)
    {
        // this->n_log_regs    =   n_log_regs;
        // this->n_phys_regs   =   n_phys_regs;
        // this->n_branches    =   n_branches;
        // this->n_active      =   n_active;

        RMT.resize(n_log_regs);
        AMT.resize(n_log_regs);

        FreeList FL;
        i64t flsize = n_phys_regs - n_log_regs;
        FL.FL_Size = flsize;
        FL.FL_entries.resize(flsize);
        FL.head = 0;
        FL.tail = flsize-1;
        FL.h_phase = FL.t_phase = false;

        ActiveList AL;
        AL.AL_size =  n_phys_regs-n_log_regs;
        AL.AL_entries.resize(AL.AL_size);
        AL.head =AL.tail =0;

        PRF.resize(n_phys_regs);
        PRF_bits.resize(n_phys_regs);
        
    }




