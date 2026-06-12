#include "branchsim.hpp"

static std::vector<std::vector<Counter_t>> predictor_table;
static std::vector<bool> GHR;
static uint64_t num_correct;
static int P;
static int H;

void gselect_init_predictor(branchsim_conf_t *sim_conf) {
    // create Counter table of 2^(P+H) counters and init to weakly not taken
    P = sim_conf->P;
    H = sim_conf->H;

    predictor_table.resize(1 << H, std::vector<Counter_t>(1 << P));
    // initialize GHR to 0
    GHR.resize(H, false);

    for (auto &row : predictor_table) {
        for (auto &counter : row) {
            Counter_init(&counter, 2);
        }
    }

#ifdef DEBUG
    printf("GSelect: Creating a Counter table of %" PRIu64 " 2-bit saturating counters\n", 0ul);
#endif
}

bool gselect_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tGSelect: Predicting... \n");
#endif

    uint64_t pc_index = (br->ip >> 2) & ((1 << P) - 1);
    uint64_t ghr_index = 0;

    for (size_t i = 0; i < H; ++i) {
        ghr_index = (ghr_index << 1) | GHR[i];
    }

    return Counter_isTaken(&predictor_table[ghr_index][pc_index]);

#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", Prediction: %d\n", 0ul, 0ul, 0);
#endif
    return false;
}

void gselect_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tGSelect: Updating based on actual behavior: %d\n", (int) br->is_taken);
#endif

    uint64_t pc_index = (br->ip >> 2) & ((1 << P) - 1);
    uint64_t ghr_index = 0;

    for (size_t i = 0; i < H; ++i) {
        ghr_index = (ghr_index << 1) | GHR[i];
    }

    Counter_update(&predictor_table[ghr_index][pc_index], br->is_taken);

    for (int i = H-1; i > 0; i--) {
        GHR[i] = GHR[i-1];
    }
    GHR[0] = br->is_taken;

#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", New Counter value: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n", 0ul, 0ul, 0ul, 0ul); // TODO: FIX ME
#endif
}

void gselect_cleanup_predictor() {
    // nothing vectors are self freeing
}

void gsplit_init_predictor(branchsim_conf_t *sim_conf) {
    // create Counter table of 2^(2*H-P) counters and init to weakly not taken

    // initialize GHR to 0

#ifdef DEBUG
    printf("GSplit: Creating a Counter table of %" PRIu64 " 2-bit saturating counters\n", 0ul);
#endif
}

bool gsplit_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tGSplit: Predicting... \n");
#endif


#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", Prediction: %d\n", 0ul, 0ul, 0);
#endif
    return false;
}

void gsplit_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tGSplit: Updating based on actual behavior: %d\n", (int) br->is_taken);
#endif


#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", New Counter value: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n",0ul, 0ul, 0ul, 0ul); // TODO: FIX ME
#endif
}

void gsplit_cleanup_predictor() {
}

/**
 *  Function to update the branchsim stats per prediction
 *
 *  @param prediction The prediction returned from the predictor's predict function
 *  @param br Pointer to the branch_t that is being predicted -- contains actual behavior
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_update_stats(bool prediction, branch_t *br, branchsim_stats_t *sim_stats) {
    
    sim_stats->num_branch_instructions++;
    if (prediction == br->is_taken) {
        sim_stats->num_branches_correctly_predicted++;
    } else {
        sim_stats->num_branches_mispredicted++;
    }

}

/**
 *  Function to cleanup branchsim statistic computations such as prediction rate, etc.
 *
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_finish_stats(branchsim_stats_t *sim_stats) {

    sim_stats->fraction_branch_instructions = (double)sim_stats->num_branch_instructions / (double)sim_stats->total_instructions;
    sim_stats->prediction_accuracy = (double)sim_stats->num_branches_correctly_predicted / (double)sim_stats->num_branch_instructions;

}


