#include "renamer.h"
#include <vector>
#include <iostream>
#include <bits/stdc++.h>



#define foru(i,n) for(uint64_t i = 0; i<n; i++)

renamer::renamer(uint64_t n_log_regs,uint64_t n_phys_regs,uint64_t n_branches,uint64_t n_active)
{   
    ///////////////assertion///////////////////////
    assert(n_phys_regs>n_log_regs);
    assert(n_branches >= 1 && n_branches <= 64);
    assert(n_active > 0);

    ///////////////vector allocation/////////////////
    RMT.resize(n_log_regs);
    AMT.resize(n_log_regs);
 
    foru(i,n_log_regs){
        RMT[i] = i;
        AMT[i] = i;
    }

    ////////////////Free list allocation//////////////
    i64t flsize = n_phys_regs - n_log_regs;
    FL.FL_Size = flsize;
    FL.FL_entries.resize(flsize);
    FL.head =   FL.tail   =   0;
    FL.h_phase  =    0;
    FL.t_phase  =    1 ;

    foru(j,flsize)
    {
        FL.FL_entries[j] = j+n_log_regs; 
    }

    ////////////////Active list allocation/////////////
    AL.AL_size  =   n_active;
    AL.AL_entries.resize(n_active);
    AL.head    =    AL.tail    =   0;
    AL.h_phase  =   AL.t_phase  =   0;

    //////////////PRF and PRF bits allocation///////////
    PRF.resize(n_phys_regs);
    PRF_bits.resize(n_phys_regs);
    foru(i,n_phys_regs)
    {
        PRF[i] = 0; 
        PRF_bits[i] = 1;
    }

    //////////////GBM set///////////////////////////////
    GBM = 0;

    
    
    ///////////////Private variables////////////////////
    number_of_branches      =   n_branches;
    number_of_logical_reg   =   n_log_regs;
    number_of_physical_reg  =   n_phys_regs;
    total_active_instruction=   n_active;

    /////////////////Branch Checkpoint allocation///////
    Branch_CheckPoint.resize(number_of_branches);
    
}

bool renamer::stall_reg(uint64_t bundle_dst)
{
//     static int i = 0;
// if(i > 3)
//     cout << "hello" << endl;
//     i+= bundle_dst;
//     cout << "REG ENTER BUNDLE SIZE  " << bundle_dst << " head "<<FL.head<<"  tail : "<< FL.tail<<  endl;
    i64t n_freelist_enteries = enteries_in_freelist();

    if(bundle_dst>n_freelist_enteries)
    {
        cout<<"stall_reg is true\n";

        return true;
    }
    cout<<"stall_reg is false\n";
    return false;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
    //check number of set bits of GBM (number of 1)
    i64t count_set_bits    =    0;
    i64t t_GBM = GBM;
    foru(i,number_of_branches)
    {
        if((t_GBM & 1) == true)   count_set_bits++;
        t_GBM  =t_GBM>>1;
    }   
   
    if((number_of_branches-count_set_bits)<bundle_branch)    
    {
    cout<<"stall_branch is true\n";

        return true;
    }
    cout<<"stall_branch is false\n";
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
    cout<<"inside rename destination";
    i64t rmt_value = FL.FL_entries[FL.head];
    cout << rmt_value << endl;
    //increment the head 
    FL.head++;
    //wrap around
    if(FL.head == FL.FL_Size)
    {
        FL.head = 0;
        FL.h_phase = !FL.h_phase;
    }
    RMT[log_reg] = rmt_value;
    cout << "return rename destination"<<"\n";
    return rmt_value;

}

uint64_t renamer::checkpoint()
{
    //find the branch id position inside gbm if there is one
    i64t pos = 0;
    i64t t_GBM = GBM;

    foru(i,number_of_branches)
    {
        if((t_GBM & 1) == 0 )
        {
            cout << "checkpoint found" << endl;
            pos = i;
            break;
        }
        t_GBM  >>= 1;
    }
    
    GBM = (GBM | (1<<pos)); //set the bit to 1
    Branch_CheckPoint[pos].SMT = RMT;
    Branch_CheckPoint[pos].checkpoint_freelist_head = FL.head;
    Branch_CheckPoint[pos].checkpoint_freelist_head_phase = FL.h_phase;
    Branch_CheckPoint[pos].checkpoint_GBM = GBM;
    cout << "checkpoint " << pos << endl;
    return  pos; //this is branch id
    // return 0;
}

bool renamer::stall_dispatch(uint64_t bundle_inst)
{
    cout << "dispatch enter data\n";
    // find the free space inside the al
    
    i64t AL_free_space = space_in_activelist();
    cout << "Dispatch BUNDLE SIZE " << bundle_inst << " head "<<AL.head<<"  tail : "<< AL.tail<<  endl;

    if(AL_free_space<bundle_inst)
    {
        //cout<<"alfree_space"<<AL.AL_size-enteries_in_activelist()<<"\n";
        cout<<"dispatch return true  "<<bundle_inst<<"\n";
        return true;
    }
    cout<<"stall dispatch is false \n";
    return false;

    
}


uint64_t renamer::dispatch_inst(bool dest_valid,uint64_t log_reg,uint64_t phys_reg,bool load,bool store,bool branch,bool amo,bool csr,uint64_t PC)
{
    i64t index_of_instruction = AL.tail;
    
    //1. dest_valid if true then the instr. has a destination register.

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
    AL.AL_entries[AL.tail].dest_flag    =   dest_valid;
    AL.AL_entries[AL.tail].complete_bit =   0;
    AL.AL_entries[AL.tail].branch_misp_bit = 0 ;
    AL.AL_entries[AL.tail].load_viol_bit = 0;
    AL.AL_entries[AL.tail].exception_bit = 0;
    AL.AL_entries[AL.tail].value_misp_bit = 0;


    AL.AL_entries[AL.tail].prog_counter =   PC;


    //increment the tail
    AL.tail++;
    //check for wrap around condition.
    if(AL.tail == AL.AL_size)
    {
        AL.tail = 0;
        AL.t_phase = !AL.t_phase;
    }
    cout<<"inside dispatch instruction "<<"\n";

    return index_of_instruction;
}


bool renamer::is_ready(uint64_t phys_reg)
{
    bool checker;
    checker = PRF_bits[phys_reg];
    return(checker == 1);
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
    PRF_bits[phys_reg]   =   true;
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
        GBM &= (~(1<<branch_ID));
        //clear the branch bit in all the branch checkpoints
        foru(i, number_of_branches)
        {
            Branch_CheckPoint[i].checkpoint_GBM &= (~(1<<branch_ID));
        }
    }
    // check it later
    else
    {
        // * Restore the GBM from the branch's checkpoint.
        Branch_CheckPoint[branch_ID].checkpoint_GBM &= (~(1<<branch_ID));
        GBM = Branch_CheckPoint[branch_ID].checkpoint_GBM;
        //assertion is needed here to make sure.

        //restore the rmt from branch checkpoint
        RMT = Branch_CheckPoint[branch_ID].SMT;

        //restore the free list head and the phase
        FL.head = Branch_CheckPoint[branch_ID].checkpoint_freelist_head;
        FL.h_phase = Branch_CheckPoint[branch_ID].checkpoint_freelist_head_phase;
        
        //restore the active list tail and its phase bit
        AL.tail = AL_index+1;
        if(AL.tail == AL.AL_size)
        {
            AL.tail = 0;
        }
        //restore phase
        AL.t_phase  =   !AL.h_phase;
        if(AL.tail>AL.head)
        {
            AL.t_phase = AL.h_phase;
        }
    }

}
 

bool renamer::precommit(bool &completed,bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,bool &load, bool &store, bool &branch, bool &amo, bool &csr,uint64_t &PC)
{
    if((AL.head == AL.tail) && (AL.h_phase == AL.t_phase))
    {
        //active list is empty
         cout<<"active list is empty"<<AL.AL_size<<"\n";
        // cout<<AL.AL_size<<"\t"<<enteries_in_activelist()<<"\n";
        
        return false;
    }
    else
    {
        completed   =   AL.AL_entries[AL.head].complete_bit; 
        exception   =   AL.AL_entries[AL.head].exception_bit; 
        load_viol   =   AL.AL_entries[AL.head].load_viol_bit;
        br_misp     =   AL.AL_entries[AL.head].branch_misp_bit;
        val_misp    =   AL.AL_entries[AL.head].value_misp_bit;
        load        =   AL.AL_entries[AL.head].load_flag;
        store       =   AL.AL_entries[AL.head].store_flag;
        branch      =   AL.AL_entries[AL.head].branch_flag;
        amo         =   AL.AL_entries[AL.head].atomic_flag;
        csr         =   AL.AL_entries[AL.head].CSR_flag;
        PC          =   AL.AL_entries[AL.head].prog_counter;
        // cout<<"precommint is true\n";
        //  cout<<"AL return true " << AL.AL_size<<"\t"<<enteries_in_activelist()<<"\n";

        return true;

    }
}

void renamer::commit()
{
    // assert(AL.AL_size !=0);
    assert(AL.AL_entries[AL.head].complete_bit == true);
    assert(AL.AL_entries[AL.head].exception_bit != true);
    assert(AL.AL_entries[AL.head].load_viol_bit != true);
    /*if(AL.AL_entries[AL.head].branch_flag)
    {
        cout<<"it is a branch\n";
    }*/
    //commiting the head of the AL; ALso means freeing the register
    //check if phy dest reg is there or not
    if(AL.AL_entries[AL.head].dest_flag)
    {
        //there is destination reg
        //go to AMT and free the reg there
        cout<<"Destination flag is true: tail increment"<<endl;
        FL.FL_entries[FL.tail] = AMT[AL.AL_entries[AL.head].log_dest];
        //increase the fl tail
        FL.tail++;
        if(FL.tail == FL.FL_Size) 
        {
            FL.tail = 0;
            FL.t_phase = !FL.t_phase;
        }

        //now put the physical reg from AL to AMT and increase head pointer of the AL
        AMT[AL.AL_entries[AL.head].log_dest] = AL.AL_entries[AL.head].phy_dest;
    }
        AL.head++;
        if(AL.head == AL.AL_size) 
        {
            //wrap up
            AL.head = 0;
            AL.h_phase = !AL.h_phase;
        }
        cout<<"Data got committed"<<endl;
}

void renamer::squash()
{
    //squashing all instruction first thing AMT will be copied to RMT
    RMT = AMT;
    foru(i,PRF_bits.size())
    {
        PRF_bits[i] = 1;
    }
    // Active List will be emptied and phase are matched 
    AL.tail = AL.head ;
    AL.t_phase = AL.h_phase;
    //freelist is filled and phase are mismatched
    FL.head = FL.tail;
    FL.h_phase = !FL.t_phase;
    
    cout<<"SQUASH is working";
    GBM  = 0;
    foru(i,number_of_branches)
    {
        Branch_CheckPoint[i].checkpoint_GBM = 0;
    }

}

void renamer::set_exception(uint64_t AL_index)
{
    AL.AL_entries[AL_index].exception_bit = 1;
}
void renamer::set_load_violation(uint64_t AL_index)
{
    AL.AL_entries[AL_index].load_viol_bit = 1;
}
void renamer::set_branch_misprediction(uint64_t AL_index)
{
    AL.AL_entries[AL_index].branch_misp_bit =1;

}
void renamer::set_value_misprediction(uint64_t AL_index)
{
    AL.AL_entries[AL_index].value_misp_bit = 1;
}

bool renamer::get_exception(uint64_t AL_index)
{
    return AL.AL_entries[AL_index].exception_bit;
}

uint64_t renamer::space_in_activelist()
{
    if(AL.h_phase == AL.t_phase)
    {
        return AL.AL_size - AL.tail + AL.head;   
    }
else{
        return AL.head - AL.tail;
    }
}

uint64_t renamer::enteries_in_freelist()
{
    if(FL.h_phase == FL.t_phase)
    {
        return FL.tail-FL.head;
    }
    else{
        return FL.FL_Size-FL.head+FL.tail;
    }
}
















