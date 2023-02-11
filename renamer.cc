#include "renamer.h"
#include <vector>
#include <iostream>
#include <bits/stdc++.h>

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

    AL.AL_size =  0;
    AL.AL_entries.resize(n_active);
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
    number_of_branches      =   n_branches;
    number_of_logical_reg   =   n_log_regs;
    number_of_physical_reg  =   n_phys_regs;
    total_active_instruction=   n_active;
    Branch_CheckPoint.resize(number_of_branches);
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
    //find the branch id position inside gbm if there is one
    int pos;
    if(GBM == 0)
    {
        // no branch id is available
    }
    else
    {
        pos = 1;
        foru(i,number_of_branches)
        {
            if(GBM&(1<<i))
            {
                pos++;
            }
            else break;
        }

        GBM = (GBM | (1<<pos)); //set the bit to 1
        Branch_CheckPoint[pos].SMT.assign(RMT.begin(),RMT.end());//copy the rmt to smt
        Branch_CheckPoint[pos].checkpoint_freelist_head = FL.head;
        Branch_CheckPoint[pos].checkpoint_freelist_head_phase = FL.h_phase;
        Branch_CheckPoint[pos].checkpoint_GBM = GBM;
        return  pos; //this is branch id

    }
}

bool renamer::stall_dispatch(uint64_t bundle_inst)
{
    //AL_size will keep the number of remaining
    //
    if(AL.AL_size < bundle_inst)
    {
        return true;
    }
    return false;
}


uint64_t renamer::dispatch_inst(bool dest_valid,uint64_t log_reg,uint64_t phys_reg,bool load,bool store,bool branch,bool amo,bool csr,uint64_t PC)
{
    i64t index_of_instruction = AL.tail;
    if(dest_valid)
    {
        AL.AL_entries[AL.tail].log_dest = log_reg;
        AL.AL_entries[AL.tail].phy_dest = phys_reg;

    }
    AL.AL_entries[AL.tail].load_flag    =   load;
    AL.AL_entries[AL.tail].store_flag   =   store;
    AL.AL_entries[AL.tail].branch_flag  =   branch;
    AL.AL_entries[AL.tail].atomic_flag  =   amo;
    AL.AL_entries[AL.tail].CSR_flag     =   csr;

    AL.AL_entries[AL.tail].prog_counter =   PC;

    AL.tail++;
    return index_of_instruction;
}


bool renamer::is_ready(uint64_t phys_reg)
{
    bool checker;
    checker = PRF_bits[phys_reg];
    if(checker) return  true;
    else    return  false;
}


void renamer::clear_ready(uint64_t phys_reg)
{
    PRF_bits[phys_reg] = false;
}

uint64_t renamer::read(uint64_t phys_reg)
{
    return PRF[phys_reg];
}

void renamer::set_ready(uint64_t phys_reg)
{
    PRF[phys_reg]   =   true;
}

void renamer::write(uint64_t phys_reg, uint64_t value)
{
    PRF[phys_reg] = value;
}

void renamer::set_complete(uint64_t AL_index)
{
    AL.AL_entries[AL_index].complete_bit = true;
}

void renamer::resolve(uint64_t AL_index,uint64_t branch_ID,bool correct)
{
    //resolving branch here.
    //branch is correctly predicted
    if(correct)
    {
        // clear the branch bit in the gbm
        GBM &= ~(1<<branch_ID);
        //clear the branch bit in all the branch checkpoints
        foru(i, number_of_branches)
        {
            Branch_CheckPoint[i].checkpoint_GBM &= ~(1<<branch_ID);
        }
    }
    else
    {
        // * Restore the GBM from the branch's checkpoint.
        GBM = Branch_CheckPoint[branch_ID].checkpoint_GBM;
        //assertion is needed here to make sure.

        //restore the rmt from branch checkpoint
        RMT.assign(Branch_CheckPoint[branch_ID].SMT.begin(),Branch_CheckPoint[branch_ID].SMT.end());

        //restore the free list 
    }

}


















