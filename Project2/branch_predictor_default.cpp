#include "branchsim.hpp"

#include "Counter.hpp"

void always_taken_init_predictor(branchsim_conf_t *sim_conf) {

}

bool always_taken_predict(branch_t *br) {
    return true;
}

void always_taken_update_predictor(branch_t *br) {

}

void always_taken_cleanup_predictor() {

}