__kernel void cross_check(__global uchar* map1, __global uchar* map2, __global uchar* map, uint imsize, uint threshold) {
    int idx = get_global_id(0);
    
    if (idx < imsize) {
        if (abs((int) map1[idx] - map2[idx]) > threshold)
            map[idx] = 0;
        else
            map[idx] = map1[idx];
    }
}

