#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <deque>
#include <algorithm>

#include "procsim.hpp"

// Global state
static std::vector<PhysicalRegister> register_file;
static std::vector<uint64_t> rat;
static std::vector<ROBEntry> rob;
static std::vector<ReservationStation> scheduling_queue;
static std::deque<inst_t> dispatch_queue;

// Pipeline configuration
static size_t num_rob_entries;
static size_t num_schedq_entries_per_fu;
static size_t num_pregs;
static size_t fetch_width;
static size_t dispatch_width;
static uint64_t dispq_total = 0;
static uint64_t schedq_total = 0;
static uint64_t rob_total = 0;

#ifdef DEBUG
static void print_operand(int8_t rx) {
    if (rx < 0) {
        printf("(none)"); //  PROVIDED
    } else {
        printf("R%" PRId8, rx); //  PROVIDED
    }
}

// Useful in the fetch and dispatch stages
static void print_instruction(const inst_t *inst) {
    if (!inst) {
        return;
    }
    static const char *opcode_names[] = {NULL, NULL, "ADD", "MUL", "LOAD", "STORE", "BRANCH"};

    printf("opcode=%s, dest=", opcode_names[inst->opcode]); // PROVIDED
    print_operand(inst->dest); // PROVIDED
    printf(", src1="); // PROVIDED
    print_operand(inst->src1); // PROVIDED
    printf(", src2="); // PROVIDED
    print_operand(inst->src2); // PROVIDED
    printf(", dyncount=%lu", inst->dyn_instruction_count); //  PROVIDED
}

// This will print the state of the ROB where instructions are identified by their dyn_instruction_count
static void print_rob(void) {
    size_t printed_idx = 0;
    printf("\tAllocated Entries in ROB: %lu\n", 0ul);
    for (/* ??? */; /* ??? */ false; /* ??? */) {
        if (printed_idx == 0) {
            printf("    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", 0ul, 0, 0);
        } else if (!(printed_idx & 0x3)) {
            printf("\n    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", 0ul, 0, 0);
        } else {
            printf(", { dyncount=%05" PRIu64 " completed: %d, mispredict: %d }", 0ul, 0, 0);
        }
        printed_idx++;
    }
    if (!printed_idx) {
        printf("    (ROB empty)");
    }
    printf("\n");
}

// This will print out the state of the RAT
static void print_rat(void) {
    for (uint64_t regno = 0; regno < NUM_REGS; regno++) {
        if (regno == 0) {
            printf("    { R%02" PRIu64 ": P%03" PRIu64 " }", regno, 0ul);
        } else if (!(regno & 0x3)) {
            printf("\n    { R%02" PRIu64 ": P%03" PRIu64 " }", regno, 0ul);
        } else {
            printf(", { R%02" PRIu64 ": P%03" PRIu64 " }", regno, 0ul);
        }
    }
    printf("\n");
}

// This will print out the state of the register file, where P0-P31 are architectural registers 
// and P32 is the first PREG 
static void print_prf(void) {
    for (uint64_t regno = 0; /* ??? */ false; regno++) {
        if (regno == 0) {
            printf("    { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, 0, 0);
        } else if (!(regno & 0x3)) {
            printf("\n    { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, 0, 0);
        } else {
            printf(", { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, 0, 0);
        }
    }
    printf("\n");
}
#endif


// Optional helper function which retires instructions in ROB in program
// order. (In a real system, the destination register value from the ROB would be written to the
// architectural registers, but we have no register values in this
// simulation.) This function returns the number of instructions retired.
// Immediately after retiring a mispredicting branch, this function will set
// *retired_mispredict_out = true and will not retire any more instructions. 
// Note that in this case, the mispredict must be counted as one of the retired instructions.
// During mispredict, reset the register files and RAT to initial values.
static uint64_t stage_state_update(procsim_stats_t *stats, bool *retired_mispredict_out) {
    uint64_t retired = 0;
    *retired_mispredict_out = false;

    for (auto& entry : rob) {
        if (!entry.completed) {
            break;
        }

        if (entry.opcode == OPCODE_BRANCH) {
            for (const auto& rs : scheduling_queue) {
                if (rs.instruction.dyn_instruction_count == entry.dyn_instruction_count) {
                    procsim_driver_update_predictor(rs.instruction.pc, rs.instruction.br_taken, entry.dyn_instruction_count);
                    break;
                }
            }
        }

        if (entry.dest >= 0 && entry.prev_preg >= NUM_REGS) {
            register_file[entry.prev_preg].free = true;
        }
        
        if (entry.mispredict) {
            *retired_mispredict_out = true;
            
            for (size_t i = 0; i < register_file.size(); i++) {
                if (i < NUM_REGS) {
                    register_file[i].free = false;
                    register_file[i].ready = true;
                } else {
                    register_file[i].free = true;
                    register_file[i].ready = false;
                }
            }

            for (size_t i = 0; i < NUM_REGS; i++) {
                rat[i] = i;
            }

            retired++;
            break;
        }

        // Clear ROB entry
        entry.dyn_instruction_count = 0;
        entry.completed = false;
        retired++;
    }

#ifdef DEBUG
    printf("Stage State Update: \n");
#endif
    return retired;
}

// Optional helper function which is responsible for moving instructions
// through pipelined Function Units and then when instructions complete (that
// is, when instructions are in the final pipeline stage of an FU and aren't
// stalled there), setting the ready bits in the register file. This function 
// should remove an instruction from the scheduling queue when it has completed.
static void stage_exec(procsim_stats_t *stats) {
    for (auto& rs : scheduling_queue) {
        if (!rs.busy || rs.cycles_remaining == 0) {
            continue;
        }

        rs.cycles_remaining--;

        if (rs.cycles_remaining == 0) {
            if (rs.instruction.dest >= 0) {
                register_file[rs.dest_preg].ready = true;
            }

            for (auto& rob_entry : rob) {
                if (rob_entry.dyn_instruction_count == rs.instruction.dyn_instruction_count) {
                    rob_entry.completed = true;
                    break;
                }
            }

            rs.busy = false;
        }
    }
#ifdef DEBUG
    printf("Stage Exec: \n");
#endif

#ifdef DEBUG
    printf("\tProgressing ALU units\n");
#endif

    // Progress MULs
#ifdef DEBUG
    printf("\tProgressing MUL units\n");
#endif

    // Progress LSU loads / stores
#ifdef DEBUG
    printf("\tProgressing LSU units\n"); 
#endif

    // Apply Result Busses
#ifdef DEBUG
    printf("\tProcessing Result Buses\n"); // PROVIDED
#endif

}

// Optional helper function which is responsible for looking through the
// scheduling queue and firing instructions that have their source pregs
// marked as ready. Note that when multiple instructions are ready to fire
// in a given cycle, they must be fired in program order. 
// Also, load and store instructions must be fired according to the 
// memory disambiguation algorithm described in the assignment PDF. Finally,
// instructions stay in their reservation station in the scheduling queue until
// they complete (at which point stage_exec() above should free their RS).

static bool compare_by_program_order(const ReservationStation* a, const ReservationStation* b) {
    return a->instruction.dyn_instruction_count < b->instruction.dyn_instruction_count;
}

static void stage_schedule(procsim_stats_t *stats) {
    std::vector<ReservationStation*> ready_insts;
    bool store_fired = false;
    
    for (auto& rs : scheduling_queue) {
        if (!rs.busy || !rs.src1_ready || !rs.src2_ready) {
            continue;
        }
    
        bool can_fire = true;

        if (rs.instruction.opcode == OPCODE_STORE) {
            if (store_fired) {
                continue;
            }

            for (const auto& rob_entry : rob) {
                if (rob_entry.dyn_instruction_count < rs.instruction.dyn_instruction_count &&
                    (rob_entry.opcode == OPCODE_LOAD || rob_entry.opcode == OPCODE_STORE) && 
                    !rob_entry.completed) {
                    can_fire = false;
                    break;
                }
            }
            if (can_fire) store_fired = true;
        }
        else if (rs.instruction.opcode == OPCODE_LOAD) {
            for (const auto& rob_entry : rob) {
                if (rob_entry.dyn_instruction_count < rs.instruction.dyn_instruction_count &&
                    rob_entry.opcode == OPCODE_STORE && !rob_entry.completed) {
                    can_fire = false;
                    break;
                }
            }
        }
    
        if (can_fire) {
            ready_insts.push_back(&rs);
        }
    }
    std::sort(ready_insts.begin(), ready_insts.end(), compare_by_program_order);

    for (auto rs : ready_insts) {
        if (rs->instruction.opcode == OPCODE_MUL) {
            rs->cycles_remaining = 3;
        }
        else if (rs->instruction.opcode == OPCODE_LOAD) {
            rs->cycles_remaining = rs->instruction.dcache_hit == CACHE_LATENCY_L1_HIT ? L1_HIT_CYCLES :
                                    rs->instruction.dcache_hit == CACHE_LATENCY_L2_HIT ? L2_HIT_CYCLES : 
                                    L2_MISS_CYCLES;
        }
        else {
            rs->cycles_remaining = 1;
        }
    }
#ifdef DEBUG
    printf("Stage Schedule: \n");
#endif
}

// Optional helper function which looks through the dispatch queue, decodes
// instructions, and inserts them into the scheduling queue. Dispatch should
// not add an instruction to the scheduling queue unless there is space for it
// in the scheduling queue and the ROB; 
// effectively, dispatch allocates reservation stations and ROB space for 
// each instruction dispatched and stalls if there any are unavailable. 
// Note the scheduling queue has a configurable size and the ROB has p + 32 entries.
// The PDF has details.
// dispatch width is fetch width
static void stage_dispatch(procsim_stats_t *stats) {
    size_t dispatched = 0;
    
    while (!dispatch_queue.empty() && dispatched < dispatch_width) {
        inst_t inst = dispatch_queue.front();
        
        if (inst.dest == -1 && inst.src1 == -1 && inst.src2 == -1) {
            dispatch_queue.pop_front();
            dispatched++;
            continue;
        }
        bool found_rs = false;
        for (auto& rs : scheduling_queue) {
            if (!rs.busy) {
                size_t rob_idx;
                bool found_rob = false;
                for (size_t i = 0; i < rob.size(); i++) {
                    if (!rob[i].completed && rob[i].dyn_instruction_count == 0) {
                        rob_idx = i;
                        found_rob = true;
                        break;
                    }
                }
                if (!found_rob) {
                    stats->rob_no_dispatch_cycles++;
                    break;
                }
      
                uint64_t dest_preg = NUM_REGS;
                if (inst.dest >= 0) {
                    bool found_preg = false;
                    for (size_t i = NUM_REGS; i < register_file.size(); i++) {
                        if (register_file[i].free) {
                            dest_preg = i;
                            register_file[i].free = false;
                            register_file[i].ready = false;
                            found_preg = true;
                            break;
                        }
                    }
                    if (!found_preg) {
                        stats->no_dispatch_pregs_cycles++;
                        break;
                    }
                }

                rob[rob_idx].dyn_instruction_count = inst.dyn_instruction_count;
                rob[rob_idx].completed = false;
                rob[rob_idx].mispredict = inst.mispredict;
                rob[rob_idx].opcode = inst.opcode;

                rs.busy = true;
                rs.instruction = inst;
                
                if (inst.src1 >= 0) {
                    rs.src1_preg = rat[inst.src1];
                    rs.src1_ready = register_file[rs.src1_preg].ready;
                }
                if (inst.src2 >= 0) {
                    rs.src2_preg = rat[inst.src2];
                    rs.src2_ready = register_file[rs.src2_preg].ready;
                }

                if (inst.dest >= 0) {
                    rs.dest_preg = dest_preg;
                    rs.prev_preg = rat[inst.dest];
                    rat[inst.dest] = dest_preg;
                }

                dispatch_queue.pop_front();
                dispatched++;
                found_rs = true;
                break;
            }
        }
        if (!found_rs) break;
    }
#ifdef DEBUG
    printf("Stage Dispatch: \n");
#endif
}

// Optional helper function which fetches instructions from the instruction
// cache using the provided procsim_driver_read_inst() function implemented
// in the driver and appends them to the dispatch queue. 
// If a NULL pointer is fetched from the procsim_driver_read_inst, and driver_read_status is DRIVER_READ_ICACHE_MISS
// insert a NOP to the dispatch queue
// that NOP should be dropped at the dispatch stage
static void stage_fetch(procsim_stats_t *stats) {
    for (size_t i = 0; i < fetch_width; i++) {
        driver_read_status_t status;
        const inst_t* inst = procsim_driver_read_inst(&status);
        
        if (status == DRIVER_READ_END_OF_TRACE) {
            break;
        }

        if (status == DRIVER_READ_MISPRED) {
            stats->instructions_fetched++;
            continue;
        }

        if (status == DRIVER_READ_ICACHE_MISS) {
            inst_t nop = {};
            nop.opcode = OPCODE_ADD;
            nop.dest = -1;
            nop.src1 = -1;
            nop.src2 = -1;
            dispatch_queue.push_back(nop);
            continue;
        }

        if (inst != nullptr) {
            dispatch_queue.push_back(*inst);
            stats->instructions_fetched++;
            if (inst->opcode == OPCODE_BRANCH) {
                stats->num_branch_instructions++;
            }
        }
    }
#ifdef DEBUG
    printf("Stage Fetch: \n");
#endif
}

// Use this function to initialize all your data structures, simulator
// state, and statistics.
void procsim_init(const procsim_conf_t *sim_conf, procsim_stats_t *stats) {
    num_rob_entries = sim_conf->num_rob_entries;
    num_schedq_entries_per_fu = sim_conf->num_schedq_entries_per_fu;
    num_pregs = sim_conf->num_pregs;
    fetch_width = sim_conf->fetch_width;
    dispatch_width = sim_conf->dispatch_width;

    register_file.resize(NUM_REGS + num_pregs);
    for (size_t i = 0; i < register_file.size(); i++) {
        if (i < NUM_REGS) {
            register_file[i].free = false;
            register_file[i].ready = true;
        } else {
            register_file[i].free = true;
            register_file[i].ready = false;
        }
    }

    rat.resize(NUM_REGS);
    for (size_t i = 0; i < NUM_REGS; i++) {
        rat[i] = i;
    }
 
    rob.resize(num_rob_entries);
 
    size_t total_fu = sim_conf->num_alu_fus + sim_conf->num_mul_fus + sim_conf->num_lsu_fus;
    scheduling_queue.resize(total_fu * num_schedq_entries_per_fu);
    
#ifdef DEBUG
    printf("\nScheduling queue capacity: %lu instructions\n", 0lu);
    printf("Initial RAT state:\n");
    print_rat();
    printf("\n");
#endif
}

// To avoid confusion, we have provided this function for you. Notice that this
// calls the stage functions above in reverse order! This is intentional and
// allows you to avoid having to manage pipeline registers between stages by
// hand. This function returns the number of instructions retired, and also
// returns if a mispredict was retired by assigning true or false to
// *retired_mispredict_out, an output parameter.
uint64_t procsim_do_cycle(procsim_stats_t *stats,
                          bool *retired_mispredict_out) {
#ifdef DEBUG
    printf("================================ Begin cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
#endif

    // stage_state_update() should set *retired_mispredict_out for us
    uint64_t retired_this_cycle = stage_state_update(stats, retired_mispredict_out);

    if (*retired_mispredict_out) {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Retired mispredict, so notifying driver to fetch correctly!\n", retired_this_cycle); //  PROVIDED
#endif

        // After we retire a misprediction, the other stages don't need to run
        stats->branch_mispredictions++;
    } else {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Did not retire mispredict, so proceeding with other pipeline stages.\n", retired_this_cycle); //  PROVIDED
#endif

        // If we didn't retire an interupt, then continue simulating the other
        // pipeline stages
        stage_exec(stats);
        stage_schedule(stats);
        stage_dispatch(stats);
        stage_fetch(stats);
    }

#ifdef DEBUG
    printf("End-of-cycle dispatch queue usage: %lu\n", 0ul);
    printf("End-of-cycle sched queue usage: %lu\n", 0ul);
    printf("End-of-cycle ROB usage: %lu\n", 0ul);
    printf("End-of-cycle RAT state:\n");
    print_rat();
    printf("End-of-cycle Physical Register file state:\n");
    print_prf();
    printf("End-of-cycle ROB state:\n");
    print_rob();
    printf("================================ End cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
    print_instruction(NULL); // this makes the compiler happy, ignore it
#endif

    size_t dispq_size = dispatch_queue.size();
    size_t schedq_size = 0;
    size_t rob_size = 0;
    for (const auto& rs : scheduling_queue) {
        if (rs.busy) schedq_size++;
    }

    for (const auto& entry : rob) {
        if (entry.dyn_instruction_count != 0) rob_size++;
    }

    stats->dispq_max_usage = std::max(stats->dispq_max_usage, dispq_size);
    stats->schedq_max_usage = std::max(stats->schedq_max_usage, schedq_size);
    stats->rob_max_usage = std::max(stats->rob_max_usage, rob_size);


    dispq_total += dispq_size;
    schedq_total += schedq_size;
    rob_total += rob_size;
    stats->cycles++;

    return retired_this_cycle;
}

void procsim_finish(procsim_stats_t *stats) {
    stats->dispq_avg_usage = (double)dispq_total / stats->cycles;
    stats->schedq_avg_usage = (double)schedq_total / stats->cycles;
    stats->rob_avg_usage = (double)rob_total / stats->cycles;
    stats->ipc = (double)stats->instructions_retired / stats->cycles;

    register_file.clear();
    rat.clear();
    rob.clear();
    scheduling_queue.clear();
    dispatch_queue.clear();
}