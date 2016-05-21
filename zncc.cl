__kernel void zncc(__global uchar *left, __global  uchar *right, __global uchar *dmap, int w, int h, int bsx, int bsy, int mind, int maxd) {
    int i = get_global_id(0);
    int j = get_global_id(1);

    int imsize = w*h; // Size of the image
    int bsize = bsx*bsy; // Block size

    int i_b, j_b; // Indices within the block
    int ind_l, ind_r; // Indices of block values within the whole image
    int d; // Disparity value
    float cl, cr; // centered values of a pixel in the left and right images;
    
    float lbmean, rbmean; // Blocks means for left and right images
    float lbstd, rbstd; // Left block std, Right block std
    float current_score; // Current ZNCC value
    
    int best_d;
    float best_score;


    // Searching for the best d for the current pixel
    best_d = maxd;
    best_score = -1;
    for (d = mind; d <= maxd; d++) {
        // Calculating the blocks' means
        lbmean = 0;
        rbmean = 0;
        for (i_b = -bsy/2; i_b < bsy/2; i_b++) {
            for (j_b = -bsx/2; j_b < bsx/2; j_b++) {
                // Borders checking
                if (!(i+i_b >= 0) || !(i+i_b < h) || !(j+j_b >= 0) || !(j+j_b < w) || !(j+j_b-d  >= 0) || !(j+j_b-d < w)) {
                        continue;
                }
                // Calculatiing indices of the block within the whole image
                ind_l = (i+i_b)*w + (j+j_b);
                ind_r = (i+i_b)*w + (j+j_b-d);
                // Updating the blocks' means
                lbmean += left[ind_l];
                rbmean += right[ind_r];
            }
        }
        lbmean /= bsize;
        rbmean /= bsize;
        
        // Calculating ZNCC for given value of d
        lbstd = 0;
        rbstd = 0;
        current_score = 0;
        
        // Calculating the numerator and the standard deviations for the denumerator
        for (i_b = -bsy/2; i_b < bsy/2; i_b++) {
            for (j_b = -bsx/2; j_b < bsx/2; j_b++) {
                // Borders checking
                if (!(i+i_b >= 0) || !(i+i_b < h) || !(j+j_b >= 0) || !(j+j_b < w) || !(j+j_b-d  >= 0) || !(j+j_b-d < w)) {
                        continue;
                }
                // Calculatiing indices of the block within the whole image
                ind_l = (i+i_b)*w + (j+j_b);
                ind_r = (i+i_b)*w + (j+j_b-d);
                    
                cl = left[ind_l] - lbmean;
                cr = right[ind_r] - rbmean;
                lbstd += cl*cl;
                rbstd += cr*cr;
                current_score += cl*cr;
            }
        }
        // Normalizing the denominator
        current_score /= sqrt(lbstd)*sqrt(rbstd);
        // Selecting the best disparity
        if (current_score > best_score) {
            best_score = current_score;
            best_d = d;
        }
    }
    dmap[i*w+j] = (uint) abs(best_d); // Considering both Left to Right and Right to left disparities
}
