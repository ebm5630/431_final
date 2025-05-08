#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>

#include "431project.h"

int count = 0;

int l1_latency_table(int size_kb, int ways) {
    int base_latency;

    // Base latency for direct-mapped (1-way)
    if      (size_kb == 8)  base_latency = 1;
    else if (size_kb == 16) base_latency = 2;
    else if (size_kb == 32) base_latency = 3;
    else if (size_kb == 64) base_latency = 4;
    else return -1;

    // Associativity adjustment
    if      (ways == 1) return base_latency;
    else if (ways == 2) return base_latency + 1;
    else if (ways == 4) return base_latency + 2;
    else return -1; // invalid associativity
}

int ul2_latency_table(int size_kb, int ways) {
    int base_latency;

    // Base latency (for 4-way set associative)
    if      (size_kb == 128)  base_latency = 7;
    else if (size_kb == 256)  base_latency = 8;
    else if (size_kb == 512)  base_latency = 9;
    else if (size_kb == 1024) base_latency = 10;
    else if (size_kb == 2048) base_latency = 11;
    else return -1;  // invalid size

    int offset;  // invalid by default

    // Associativity adjustment
    if      (ways == 1)   offset = -2;
    else if (ways == 2)   offset = -1;
    else if (ways == 4)   offset = 0;
    else if (ways == 8)   offset = 1;
    else if (ways == 16)  offset = 2;
    else return -1;  // invalid associativity

    return base_latency + offset;
}

/*
 * Not all configurations are inherently valid. (For example, L1 > L2). 
 * Returns 1 if valid, else 0.
 */
int validateConfiguration(std::string configuration){
    int config_width[4] = {1,2,4,8};
    int config_fetch_speed[2] = {1,2};
    std::string config_schedule_type[2] = {"in-order", "out-of-order"};
    int config_ruu_size[6] = {4,8,16,32,64,128};
    int config_lsq_size[4] = {4,8,16,32};
    int config_memports[2] = {1,2};
    
    int config_dl1_sets[9] = {32,64,128,256,512,1024,2048,4096,8192};
    int config_dl1_ways[3] = {1,2,4};
    int config_il1_sets[9] = {32,64,128,256,512,1024,2048,4096,8192};
    int config_il1_ways[3] = {1,2,4};

    int config_ul2_sets[10] = {256,512,1024,2048,4096,8192,16384,32768,65536,131072};
    int config_ul2_blocksize[4] = {16,32,64,128};
    int config_ul2_ways[5] = {1,2,4,8,16};

    int config_tlb_sets[5] = {4,8,16,32,64};
    int config_dl1_lat[7] = {1,2,3,4,5,6,7};
    int config_il1_lat[7] = {1,2,3,4,5,6,7};
    int config_ul2_lat[9] = {5,6,7,8,9,10,11,12,13};
    std::string config_bpredictor[6] = {"Perfect","NotTaken", "Bimodal", "2 level GAp", "2 level PAg", "Combined"};
    // is the configuration at least describing 18 integers/indices?
    if (isan18dimconfiguration(configuration) != 1)
        return 0;

        // if it is, lets convert it to an array of ints for use below
    int configurationInts[18];
    int returnValue = 1;  // assume true, set 0 if otherwise
    extractConfiguration(configuration, configurationInts); // Configuration parameters now available in array

    // Decode needed parameters
    int width = config_width[configurationInts[0]];
    int fetch_speed = config_fetch_speed[configurationInts[1]];
    std::string sched_type = config_schedule_type[configurationInts[2]];  // in-order or out-of-order
    int ruu_size = config_ruu_size[configurationInts[3]];
    int lsq_size = config_lsq_size[configurationInts[4]];
    int memports = config_memports[configurationInts[5]];

    int dl1_sets = config_dl1_sets[configurationInts[6]];
    int dl1_ways = config_dl1_ways[configurationInts[7]];
    int il1_sets = config_il1_sets[configurationInts[8]];
    int il1_ways = config_il1_ways[configurationInts[9]];

    int ul2_sets = config_ul2_sets[configurationInts[10]];
    int ul2_blocksize = config_ul2_blocksize[configurationInts[11]];
    int ul2_ways = config_ul2_ways[configurationInts[12]];

    int tlb_sets = config_tlb_sets[configurationInts[13]];
    int dl1_lat = config_dl1_lat[configurationInts[14]];
    int il1_lat = config_il1_lat[configurationInts[15]];
    int ul2_lat = config_ul2_lat[configurationInts[16]];
    std::string bpredictor = config_bpredictor[configurationInts[17]];
    
    // 
    // FIXME - YOUR VERIFICATION CODE HERE 
    // ...

    //sets the return value to 0 when the simplescalar becomes invalid

    // rule 1
    int ifq_size = width * 8;
    int il1_blocksize = ifq_size;
    int dl1_blocksize = il1_blocksize;

    int il1_size = il1_sets * il1_ways * il1_blocksize;
    int dl1_size = dl1_sets * dl1_ways * dl1_blocksize;
    int ul2_size = ul2_sets * ul2_ways * ul2_blocksize;

    // rule 2
    if (ul2_blocksize < 2 * il1_blocksize || ul2_blocksize > 128) return 0;

    if (ul2_size < (il1_size + dl1_size)) return 0;

    // rule 3
    int il1_lat_expected = l1_latency_table(il1_size / 1024, il1_ways);
    int dl1_lat_expected = l1_latency_table(dl1_size / 1024, dl1_ways);
    if (il1_lat != il1_lat_expected || dl1_lat != dl1_lat_expected) return 0;
    
    // rule 4
    int ul2_lat_expected = ul2_latency_table(ul2_size / 1024, ul2_ways);
    if (ul2_lat != ul2_lat_expected) return 0;

    return returnValue;
}




/*
 * Given the current best known configuration for a particular optimization, 
 * the current configuration, and using globally visible map of all previously 
 * investigated configurations, suggest a new, previously unexplored design 
 * point. You will only be allowed to investigate 1000 design points in a 
 * particular run, so choose wisely. Using the optimizeForX variables,
 * propose your next configuration provided the optimiztion strategy.
 */

 std::string YourProposalFunction(
    std::string currentconfiguration,
    std::string bestED2Pconfiguration,
    std::string bestEDPconfiguration,
    std::string bestEDAPconfiguration,
    std::string bestED2APconfiguration,
    int optimizeforED2P,
    int optimizeforEDP,
    int optimizeforEDAP,
    int optimizeforED2AP
) {

    /*
    algorithm objective:
    1) choose a random configuration
    2) look if valid, if not choose a different configuration

    1) EDP, use (width = 1, speed = 2, In-order, perfect predictor) and all other variables will be randomized
    2) ED2P, use (width = 8, speed = 2, Out-order, RUU Size = 128, LSQ Size = 32, Memport = 2, perfect predictor) and all other variables will be randomized
    3) EDAP, use (width = 1, speed = 2, In-order, RUU Size = 4, LSQ Sizq = 4, random predictor except perfect)
    4) ED2AP, use (width = 8, speed = 2, Out-order, Memport = 2, random predictor except perfect) and all other variables will be randomized
    */
    

    // produces an random proposal
    int configuration[18];
    do {
        // Choose configuration based on which metric is being optimized
        for(int dim = 0; dim < 18; dim++){
            configuration[dim] = rand() % GLOB_dimensioncardinality[dim];
        }

        if (optimizeforEDP) {
            configuration[0] = 0;
            configuration[1] = 1;
            configuration[2] = 0;
            configuration[17] = 0;
        } else if (optimizeforED2P) {
            configuration[0] = 3;
            configuration[1] = 1;
            configuration[2] = 1;
            configuration[3] = 5;
            configuration[4] = 3;
            configuration[5] = 1; //memports helps with delay while hurting energy
            configuration[17] = 0;
        } else if (optimizeforEDAP) {
            configuration[0] = 0;
            configuration[1] = 1;
            configuration[2] = 0;
            configuration[3] = 0;
            configuration[4] = 0;
            if (configuration[17] == 0) {
                configuration[17] = 1;
            }
        } else if (optimizeforED2AP) {
            configuration[0] = 3;
            configuration[1] = 1;
            configuration[2] = 1;
            configuration[5] = 1; 
            if (configuration[17] == 0) {
                configuration[17] = 1;
            }
        } else {
            return GLOB_baseline;
        }

    } while (validateConfiguration(compactConfiguration(configuration)) == 0);

    
    return compactConfiguration(configuration);
}


// #include <iostream>
// #include <sstream>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string>
// #include <sys/stat.h>
// #include <unistd.h>
// #include <algorithm>
// #include <fstream>
// #include <map>
// #include <math.h>
// #include <fcntl.h>

// #include "431project.h"


// /*
//  * Not all configurations are inherently valid. (For example, L1 > L2).
//  * Returns 1 if valid, else 0.
//  */
// int validateConfiguration(std::string configuration) {
//     // Check that string describes 18 indices
//     if (isan18dimconfiguration(configuration) != 1)
//         return 0;

//     int cfg[18];
//     extractConfiguration(configuration, cfg);

//     // DL1 and IL1 associativity must match
//     if (cfg[7] != cfg[9])
//         return 0;

//     // Compute block sizes in bytes
//     int il1_block = 8 << cfg[0];
//     int dl1_block = il1_block;
//     int ifq_block = 8 << cfg[1];
//     // IFQ block must match L1 block sizes
//     if (il1_block != ifq_block || dl1_block != ifq_block)
//         return 0;

//     // Fetch:IFQ ratio <= 2
//     int fetch_w = 1 << cfg[0];
//     int ifq_w   = 1 << cfg[1];
//     if (fetch_w > 2 * ifq_w)
//         return 0;

//     // UL2 block constraints
//     int ul2_block = 16 << cfg[11];
//     if (ul2_block < 2 * il1_block || ul2_block > 128)
//         return 0;

//     // Cache sizes
//     unsigned il1_size = (1 << cfg[8]) * (1 << cfg[9]) * il1_block;
//     unsigned dl1_size = (1 << cfg[6]) * (1 << cfg[7]) * dl1_block;
//     unsigned ul2_size = (256 << cfg[10]) * ul2_block * (1 << cfg[12]);
//     if (ul2_size < il1_size + dl1_size)
//         return 0;

//     // IL1 latency
//     int assoc_adj = (cfg[9] == 0 ? 0 : cfg[9] == 1 ? 1 : 2);
//     int expect_il1_lat = 1 + cfg[8] + assoc_adj;
//     if (cfg[15] != expect_il1_lat)
//         return 0;

//     // DL1 latency
//     assoc_adj = (cfg[7] == 0 ? 0 : cfg[7] == 1 ? 1 : 2);
//     int expect_dl1_lat = 1 + cfg[6] + assoc_adj;
//     if (cfg[14] != expect_dl1_lat)
//         return 0;

//     // UL2 latency
//     int assoc_ul2 = (cfg[12] == 0 ? -2 : cfg[12] == 1 ? -1 : cfg[12] == 2 ? 0 : cfg[12] == 3 ? 1 : 2);
//     int expect_ul2_lat = 7 + cfg[10] + assoc_ul2;
//     if (cfg[16] != expect_ul2_lat)
//         return 0;

//     // memport <= 2
//     if (cfg[13] > 2)
//         return 0;

//     // Fixed latencies: mplat (idx 3), memlat (idx 4), tlblat (idx 5)
//     if (cfg[3] != 0 || cfg[4] != 0 || cfg[5] != 0)
//         return 0;

//     // RUU and LSQ limits
//     if (((1 << cfg[6]) * 4) > 128)
//         return 0;
//     if (((1 << cfg[8]) * 4) > 32)
//         return 0;

//     // TLB entries (4-way) <= 512
//     if ((1 << cfg[13]) * 4 > 512)
//         return 0;

//     return 1;
// }


// /*
//  * Given the current best known configuration and optimization flags,
//  * propose a new, previously unexplored configuration.
//  */
// std::string YourProposalFunction(
//     std::string currentconfiguration,
//     std::string bestED2Pconfiguration,
//     std::string bestEDPconfiguration,
//     std::string bestEDAPconfiguration,
//     std::string bestED2APconfiguration,
//     int optimizeforED2P,
//     int optimizeforEDP,
//     int optimizeforEDAP,
//     int optimizeforED2AP
// ) {
//     const int MAX_ATTEMPTS = 100;
//     int configuration[18];
//     do {
//         // Choose configuration based on which metric is being optimized
//         for(int dim = 0; dim < 18; dim++){
//             configuration[dim] = rand() % GLOB_dimensioncardinality[dim];
//         }

//         if (optimizeforEDP) {
//             configuration[0] = 0;
//             configuration[1] = 1;
//             configuration[2] = 0;
//             configuration[17] = 0;
//         } else if (optimizeforED2P) {
//             configuration[0] = 3;
//             configuration[1] = 1;
//             configuration[2] = 1;
//             configuration[3] = 5;
//             configuration[4] = 3;
//             configuration[5] = 1; //memports helps with delay while hurting energy
//             configuration[17] = 0;
//         } else if (optimizeforEDAP) {
//             configuration[0] = 0;
//             configuration[1] = 1;
//             configuration[2] = 0;
//             configuration[3] = 0;
//             configuration[4] = 0;
//             if (configuration[17] == 0) {
//                 configuration[17] = 1;
//             }
//         } else if (optimizeforED2AP) {
//             configuration[0] = 3;
//             configuration[1] = 1;
//             configuration[2] = 1;
//             configuration[5] = 1; 
//             if (configuration[17] == 0) {
//                 configuration[17] = 1;
//             }
//         } else {
//             return GLOB_baseline;
//         }
//     }
// }