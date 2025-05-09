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

int l1_table(int size_kb, int ways) {
    int base_latency;
    if      (size_kb == 8)  base_latency = 1;
    else if (size_kb == 16) base_latency = 2;
    else if (size_kb == 32) base_latency = 3;
    else if (size_kb == 64) base_latency = 4;
    else return -1;

    if      (ways == 1) return base_latency;
    else if (ways == 2) return base_latency + 1;
    else if (ways == 4) return base_latency + 2;
    else return -1;
}

int ul2_table(int size_kb, int ways) {
    int base_latency;
    if      (size_kb == 128)  base_latency = 7;
    else if (size_kb == 256)  base_latency = 8;
    else if (size_kb == 512)  base_latency = 9;
    else if (size_kb == 1024) base_latency = 10;
    else if (size_kb == 2048) base_latency = 11;
    else return -1;

    int offset;
    if      (ways == 1)   offset = -2;
    else if (ways == 2)   offset = -1;
    else if (ways == 4)   offset = 0;
    else if (ways == 8)   offset = 1;
    else if (ways == 16)  offset = 2;
    else return -1;

    return base_latency + offset;
}

int validateConfiguration(std::string configuration) {
    int config_width[4] = {1,2,4,8};
    int config_fetch_speed[2] = {1,2};
    std::string config_schedule_type[2] = {"in-order","out-of-order"};
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
    std::string config_bpredictor[6] = {"Perfect","NotTaken","Bimodal","2 level GAp","2 level PAg","Combined"};

    if (isan18dimconfiguration(configuration) != 1)
        return 0;

    int configurationInts[18];
    extractConfiguration(configuration, configurationInts);

    int width         = config_width[configurationInts[0]];
    int fetch_speed   = config_fetch_speed[configurationInts[1]];
    std::string sched_type = config_schedule_type[configurationInts[2]];
    int ruu_size      = config_ruu_size[configurationInts[3]];
    int lsq_size      = config_lsq_size[configurationInts[4]];
    int memports      = config_memports[configurationInts[5]];
    int dl1_sets      = config_dl1_sets[configurationInts[6]];
    int dl1_ways      = config_dl1_ways[configurationInts[7]];
    int il1_sets      = config_il1_sets[configurationInts[8]];
    int il1_ways      = config_il1_ways[configurationInts[9]];
    int ul2_sets      = config_ul2_sets[configurationInts[10]];
    int ul2_blocksize = config_ul2_blocksize[configurationInts[11]];
    int ul2_ways      = config_ul2_ways[configurationInts[12]];
    int tlb_sets      = config_tlb_sets[configurationInts[13]];
    int dl1_lat       = config_dl1_lat[configurationInts[14]];
    int il1_lat       = config_il1_lat[configurationInts[15]];
    int ul2_lat       = config_ul2_lat[configurationInts[16]];
    std::string bpredictor = config_bpredictor[configurationInts[17]];

    int ifq_size      = width * 8;
    int il1_blocksize = ifq_size;
    int dl1_blocksize = il1_blocksize;
    int il1_size      = il1_sets * il1_ways * il1_blocksize;
    int dl1_size      = dl1_sets * dl1_ways * dl1_blocksize;
    int ul2_size      = ul2_sets * ul2_ways * ul2_blocksize;

    if (ul2_blocksize < 2 * il1_blocksize || ul2_blocksize > 128) return 0;
    if (ul2_size < (il1_size + dl1_size)) return 0;

    int il1_lat_expected = l1_table(il1_size / 1024, il1_ways);
    int dl1_lat_expected = l1_table(dl1_size / 1024, dl1_ways);
    if (il1_lat != il1_lat_expected || dl1_lat != dl1_lat_expected) return 0;

    int ul2_lat_expected = ul2_table(ul2_size / 1024, ul2_ways);
    if (ul2_lat != ul2_lat_expected) return 0;

    return 1;
}

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
    int configuration[18];
    do {
        for (int dim = 0; dim < 18; dim++) {
            configuration[dim] = rand() % GLOB_dimensioncardinality[dim];
        }

        if (optimizeforEDP) {
            // EDP: fastest fetch, smallest width, in-order, perfect branch pred
            configuration[0] = 0;
            configuration[1] = 0; // 1, 0 make fetching faster or so i think
            configuration[2] = 0;
            configuration[17] = 0;
        } else if (optimizeforED2P) {
            // ED2P: widest issue, fast fetch, out-of-order, big RUU & LSQ, 2 memports, perfect pred
            configuration[0] = 3;
            configuration[1] = 0; //1
            configuration[2] = 1;
            configuration[3] = 5;
            configuration[4] = 3;
            configuration[5] = 1;
            configuration[17] = 0;
        } else if (optimizeforEDAP) {
            // EDAP: minimal resources, fast fetch, in-order, non-perfect pred
            configuration[0] = 0;
            configuration[1] = 0; //1
            configuration[2] = 0;
            configuration[3] = 0;
            configuration[4] = 0;
            if (configuration[17] == 0)
                configuration[17] = 1;
        } else if (optimizeforED2AP) {
            // ED2AP: wide issue, fast fetch, out-of-order, 2 memports, non-perfect pred
            configuration[0] = 3;
            configuration[1] = 0;//1
            configuration[2] = 1;
            configuration[5] = 1;
            if (configuration[17] == 0)
                configuration[17] = 1;
        } else {
            // no metric return baseline 
            return GLOB_baseline;
        }
    } while (validateConfiguration(compactConfiguration(configuration)) == 0);

    // return first valid proposal 
    return compactConfiguration(configuration);
}

