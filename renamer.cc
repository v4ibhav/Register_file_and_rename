#include "renamer.h"
#include <vector>
#include <iostream>

using namespace std;

#define foru(i,n) for(uint64_t i = 0; i<n; i++)

renamer::renamer(uint64_t n_log_regs,uint64_t n_phys_regs,uint64_t n_branches,uint64_t n_active)
{
    // this->n_log_regs    =   n_log_regs;
    // this->n_phys_regs   =   n_phys_regs;
    // this->n_branches    =   n_branches;
    // this->n_active      =   n_active;

    assert(n_phys_regs>n_log_regs);
    assert(n_branches >= 1 && n_branches <= 64);
    assert(n_active > 0);

    RMT.resize(n_log_regs);
    AMT.resize(n_log_regs);

    foru(i,n_log_regs){
        RMT[i] = AMT[i] = i;
    }

    
    i64t flsize = n_phys_regs - n_log_regs;
    FL.FL_Size = flsize;
    FL.FL_entries.resize(flsize);
    FL.head = 0;
    FL.tail = flsize-1;
    FL.h_phase = FL.t_phase = false;

    foru(j,flsize)
    {
        FL.FL_entries[j] = j+n_log_regs; 
    }

    AL.AL_size =  n_active;
    AL.AL_entries.resize(AL.AL_size);
    AL.head =AL.tail =0;

    PRF.resize(n_phys_regs);
    PRF_bits.resize(n_phys_regs);
    //prf should be initialize with 1 
    //fl and al
    foru(i,n_phys_regs)
    {
        PRF[i] = i;
        PRF_bits[i] = i;
    }
    GBM = 0;

}

bool renamer::stall_reg(uint64_t bundle_dst)
{
    i64t free_space = FL.FL_Size;
    if(bundle_dst>free_space)
    {
        return true;
    }
    return false;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
    //check number of free checkpoints
    i64t count_bits;
    i64t t_GBM = GBM;
    while(t_GBM)
    {
        count_bits += t_GBM&1;
        t_GBM >>= 1;
    }
    //if bundle_branch is less then return true else ret false
    if(count_bits<bundle_branch)    return true;
    return false;   
}

uint64_t renamer::get_branch_mask()
{
    return GBM;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{
    return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{
    //goto freelist head-->get the index 
    i64t rmt_value = FL.FL_entries[FL.head];
    //pop the head and increment
    if(FL.head == FL.FL_Size-1)
    {
        FL.head = 0;
        //wrap around 
        FL.h_phase = 1;
    }
    else
    {
        FL.head++;
    }
    RMT[log_reg] = rmt_value;
    return RMT[log_reg];

}

uint64_t renamer::checkpoint()
{
    
}













