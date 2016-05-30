__kernel void occlusion(__global uchar* map, __global uchar* result, uint w, uint h, uint nsize, int imsize) {
    const int i = get_global_id(0);
    const int j = get_global_id(1);
    int i_b, j_b; // Indices within the block
    int ind_neib; // Index in the nighbourhood
    int ext;
    bool stop; // Stop flag for nearest neighbor interpolation

    // If the value of the pixel is zero, perform the occlusion filling by nearest neighbour interpolation
    result[i*w+j] = map[i*w+j];
    if(map[i*w+j] == 0) {
         // Spreading search of non-zero pixel in the neighborhood i,j
        stop = false;
        for (ext=1; (ext <= nsize/2) && (!stop); ext++) {
            for (j_b = -ext; (j_b <= ext) && (!stop); j_b++) {
                for (i_b = -ext; (i_b <= ext) && (!stop); i_b++) {
                    // Cehcking borders
                    if (!(i+i_b >= 0) || !(i+i_b < h) || !(j+j_b >= 0) || !(j+j_b < w) || (i_b==0 && j_b==0)) {
                        continue;
                    }
                     // Calculatiing indices of the block within the whole image
                    ind_neib = (i+i_b)*w + (j+j_b);
                    //If we meet a nonzero pixel, we interpolate and quit from this loop
                    if(map[ind_neib] != 0) {
                        result[i*w+j] = map[ind_neib];
                        stop = true;
                        break;
                    }
                        
                }
            }
        }
    }   
    }
