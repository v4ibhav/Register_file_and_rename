#include <inttypes.h>
#include <iostream>

#define int64	uint64_t

class renamer{
    private:
    // all the structure inside this specifier


	uint64_t GBM;



    public:

    renamer(uint64_t n_log_regs,
		uint64_t n_phys_regs,
		uint64_t n_branches,
		uint64_t n_active);
	bool stall_reg(uint64_t bundle_dst);
	bool stall_branch(uint64_t bundle_branch);
	uint64_t get_branch_mask();
	uint64_t get_branch_mask();
	uint64_t rename_rsrc(uint64_t log_reg);
	uint64_t rename_rdst(uint64_t log_reg);
	uint64_t checkpoint();
	bool stall_dispatch(uint64_t bundle_inst);
    uint64_t dispatch_inst(bool dest_valid,
	                       uint64_t log_reg,
	                       uint64_t phys_reg,
	                       bool load,
	                       bool store,
	                       bool branch,
	                       bool amo,
	                       bool csr,
	                       uint64_t PC);
	bool is_ready(uint64_t phys_reg);
	void clear_ready(uint64_t phys_reg);
	uint64_t read(uint64_t phys_reg);
	void set_ready(uint64_t phys_reg);
	void write(uint64_t phys_reg, uint64_t value);
	void set_complete(uint64_t AL_index);
    void resolve(uint64_t AL_index,
		     uint64_t branch_ID,
		     bool correct);
	bool precommit(bool &completed,
                       bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,
	               bool &load, bool &store, bool &branch, bool &amo, bool &csr,
		       uint64_t &PC);
	void commit();
	void squash();
    void set_exception(uint64_t AL_index);
	void set_load_violation(uint64_t AL_index);
	void set_branch_misprediction(uint64_t AL_index);
	void set_value_misprediction(uint64_t AL_index);
	bool get_exception(uint64_t AL_index);



};