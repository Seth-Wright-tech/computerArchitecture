#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{    
}

void MOESIF_protocol::dump (void)
{
#ifdef DEBUG
    const char *block_states[9] = {"X","I","S","E","O","M","F"};
    fprintf (stdout, "MOESIF_protocol - state: %s\n", block_states[state]);
#endif
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
        case MOESIF_CACHE_I:  do_cache_I (request); break;
        case MOESIF_CACHE_IM: do_cache_IM (request); break;
        case MOESIF_CACHE_M:  do_cache_M (request); break;
        case MOESIF_CACHE_IS: do_cache_IS (request); break;
        case MOESIF_CACHE_S: do_cache_S (request); break;
        case MOESIF_CACHE_SM: do_cache_SM (request); break;
        case MOESIF_CACHE_O: do_cache_O (request); break;
        case MOESIF_CACHE_OM: do_cache_OM (request); break;
        case MOESIF_CACHE_F: do_cache_F (request); break;
        case MOESIF_CACHE_FM: do_cache_FM (request); break;
        case MOESIF_CACHE_E: do_cache_E (request); break;
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

void MOESIF_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
        case MOESIF_CACHE_I:  do_snoop_I (request); break;
        case MOESIF_CACHE_IM: do_snoop_IM (request); break;
        case MOESIF_CACHE_M:  do_snoop_M (request); break;
        case MOESIF_CACHE_SM:  do_snoop_SM (request); break;
        case MOESIF_CACHE_S:  do_snoop_S (request); break;
        case MOESIF_CACHE_IS:  do_snoop_IS (request); break;
        case MOESIF_CACHE_O:  do_snoop_O (request); break; 
        case MOESIF_CACHE_OM:  do_snoop_OM (request); break;
        case MOESIF_CACHE_F:  do_snoop_F (request); break;
        case MOESIF_CACHE_FM:  do_snoop_FM (request); break;
        case MOESIF_CACHE_E:  do_snoop_E (request); break;
    default:
    	fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

inline void MOESIF_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_GETS(request->addr);
            state = MOESIF_CACHE_IS;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOESIF_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 1");
            fprintf(stdout, "ERROR 1");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOESIF_CACHE_SM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 2");
            fprintf(stdout, "ERROR 2");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 3");
            fprintf(stdout, "ERROR 3");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOESIF_CACHE_FM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 22");
            fprintf(stdout, "ERROR 22");
            fatal_error ("Client: F state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M;
            Sim->silent_upgrades++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 23");
            fprintf(stdout, "ERROR 23");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_IM (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR 4");
            fprintf(stdout, "ERROR 4");
            fatal_error("Should only have one outstanding request per processor!");
        default:
            request->print_msg (my_table->moduleID, "ERROR 5");
            fprintf(stdout, "ERROR 5");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_IS (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR 6");
            fprintf(stdout, "ERROR 6");
            fatal_error("Should only have one outstanding request per processor!");
        default:
            request->print_msg (my_table->moduleID, "ERROR 7");
            fprintf(stdout, "ERROR 7");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_SM (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR 8");
            fprintf(stdout, "ERROR 8");
            fatal_error("Should only have one outstanding request per processor!");
        default:
            request->print_msg (my_table->moduleID, "ERROR 9");
            fprintf(stdout, "ERROR 9");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOESIF_CACHE_OM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 10");
            fprintf(stdout, "ERROR 10");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_OM (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR 11");
            fprintf(stdout, "ERROR 11");
            fatal_error("Should only have one outstanding request per processor!");
        default:
            request->print_msg (my_table->moduleID, "ERROR 12");
            fprintf(stdout, "ERROR 12");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_cache_FM (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR 13");
            fprintf(stdout, "ERROR 13");
            fatal_error("Should only have one outstanding request per processor!");
        default:
            request->print_msg (my_table->moduleID, "ERROR 14");
            fprintf(stdout, "ERROR 14");
            fatal_error ("Client: FM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        case GETM:
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 13");
            fprintf(stdout, "ERROR 13");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            set_shared_line();
            break;
        case GETM:
            state = MOESIF_CACHE_I;
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 14");
            fprintf(stdout, "ERROR 14");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_M (Mreq *request)
{   
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr,request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_O;
            break;
        case GETM:
    	    send_DATA_on_bus(request->addr,request->src_mid);
		    Sim->cache_to_cache_transfers++;
    	    state = MOESIF_CACHE_I;
    	    break;
        case DATA:
            request->print_msg (my_table->moduleID, "ERROR 15");
            fprintf(stdout, "ERROR 15");
    	    fatal_error ("Should not see data for this line!  I have the line!");
    	break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 16");
            fprintf(stdout, "ERROR 16");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_IM (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        case GETM:
            break;
        case DATA:
            if (request->dest_mid == my_table->moduleID) {
                send_DATA_to_proc(request->addr);
                state = MOESIF_CACHE_M;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 17");
            fprintf(stdout, "ERROR 17");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_IS (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        case GETM:
            break;
        case DATA:
            if (request->dest_mid == my_table->moduleID) {
                send_DATA_to_proc(request->addr);
                if (get_shared_line()) {
                    state = MOESIF_CACHE_S;
                } else {
                    state = MOESIF_CACHE_E;
                }
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 18");
            fprintf(stdout, "ERROR 18");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_SM (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        case GETM:
            break;
        case DATA:
            if (request->dest_mid == my_table->moduleID) {
                send_DATA_to_proc(request->addr);
                state = MOESIF_CACHE_M;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 19");
            fprintf(stdout, "ERROR 19");
            fatal_error ("Client: SM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_O (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I;
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 20");
            fprintf(stdout, "ERROR 20");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_OM (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_IM;
            break;
        case DATA:
            if (request->dest_mid == my_table->moduleID) {
                send_DATA_to_proc(request->addr);
                state = MOESIF_CACHE_M;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 21");
            fprintf(stdout, "ERROR 21");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_F (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I;
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fprintf(stdout, "ERROR");
            fatal_error ("Client: F state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_F;
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I;
            break;
        case DATA:
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fprintf(stdout, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MOESIF_protocol::do_snoop_FM (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_IM;
            break;
        case DATA:
            if (request->dest_mid == my_table->moduleID) {
                send_DATA_to_proc(request->addr);
                state = MOESIF_CACHE_M;
            }
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR 21");
            fprintf(stdout, "ERROR 21");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}




