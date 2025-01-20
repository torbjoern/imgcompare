#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char U8;

// Structure to hold RGBA values
typedef struct {
    U8 r, g, b, a;
} Pixel;

// Structure to hold difference information
typedef struct {
    int x, y;
    double diff_magnitude;
    int largest_channel_diff;
    Pixel pixel_a;
    Pixel pixel_b;
} DiffInfo;

// Calculate Channel Diff
U8 max_channel_diff(Pixel a, Pixel b) {
    U8 dr = abs(a.r - b.r);
    U8 dg = abs(a.g - b.g);
    U8 db = abs(a.b - b.b);
    U8 da = abs(a.a - b.a);
    U8 m = max(dr,dr);
    m = max(m,dg);
    m = max(m,db);
    m = max(m,da);
    return m;
}


// Calculate Euclidean distance between two RGBA values
// https://en.wikipedia.org/wiki/Color_difference
double pixel_difference(Pixel a, Pixel b) {
    double dr = (double)fabs(a.r - b.r);
    double dg = (double)fabs(a.g - b.g);
    double db = (double)fabs(a.b - b.b);
    //double da = (double)fabs(a.a - b.a);
    return sqrt(dr*dr + dg*dg + db*db /*+ da*da*/);
}

// Blend pixel with white
Pixel wash_out_pixel(Pixel p, float wash_amount) {
    Pixel result;
    result.r = p.r + (U8)((255 - p.r) * wash_amount);
    result.g = p.g + (U8)((255 - p.g) * wash_amount);
    result.b = p.b + (U8)((255 - p.b) * wash_amount);
    result.a = p.a;
    return result;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <image1.tga> <image2.tga> <diff_output.tga>\n", argv[0]);
        return 1;
    }

    int width1, height1, channels1;
    int width2, height2, channels2;
    
    // Load the images
    unsigned char *img1 = stbi_load(argv[1], &width1, &height1, &channels1, 4);
    unsigned char *img2 = stbi_load(argv[2], &width2, &height2, &channels2, 4);

    if (!img1 || !img2) {
        printf("Error loading images\n");
        if (img1) stbi_image_free(img1);
        if (img2) stbi_image_free(img2);
        return 1;
    }

    // Check if dimensions match
    if (width1 != width2 || height1 != height2) {
        printf("Error: Images have different dimensions\n");
        printf("Image 1: %dx%d\n", width1, height1);
        printf("Image 2: %dx%d\n", width2, height2);
        stbi_image_free(img1);
        stbi_image_free(img2);
        return 1;
    }

    // Create output diff image
    unsigned char *diff_img = (unsigned char *)malloc(width1 * height1 * 4);
    if (!diff_img) {
        printf("Error allocating memory for diff image\n");
        stbi_image_free(img1);
        stbi_image_free(img2);
        return 1;
    }

    int different_pixels = 0;
    DiffInfo largest_diff = {0, 0, 0, 0};
    const float wash_out_amount = 0.5f;  // Amount to blend with white for unchanged pixels

    // Compare pixels and generate diff image
    for (int y = 0; y < height1; y++) {
        for (int x = 0; x < width1; x++) {
            int idx = (y * width1 + x) * 4;
            
            Pixel p1 = {img1[idx], img1[idx+1], img1[idx+2], img1[idx+3]};
            Pixel p2 = {img2[idx], img2[idx+1], img2[idx+2], img2[idx+3]};
            
            double diff = pixel_difference(p1, p2);
            
            // Create diff visualization
            if (diff > 0) {
                different_pixels++;
                
                // Mark different pixels in red
                diff_img[idx] = 255;     // R
                diff_img[idx+1] = 0;     // G
                diff_img[idx+2] = 0;     // B
                diff_img[idx+3] = 255;   // A
                
                if (diff > largest_diff.diff_magnitude) {
                    largest_diff.diff_magnitude = diff;
                    largest_diff.largest_channel_diff = max_channel_diff(p1,p2);
                    largest_diff.x = x;
                    largest_diff.y = y;
                    largest_diff.pixel_a = p1;
                    largest_diff.pixel_b = p2;
                }
            } else {
                // Wash out unchanged pixels
                Pixel washed = wash_out_pixel(p1, wash_out_amount);
                diff_img[idx] = washed.r;
                diff_img[idx+1] = washed.g;
                diff_img[idx+2] = washed.b;
                diff_img[idx+3] = washed.a;
            }
        }
    }

    // Save diff image
    if (!stbi_write_tga(argv[3], width1, height1, 4, diff_img)) {
        printf("Error writing diff image\n");
    } else {
        printf("Diff image saved to: %s\n", argv[3]);
    }

    // Print results
    printf("Number of different pixels: %d. Max channel diff: %d\n", different_pixels, largest_diff.largest_channel_diff);
    if (different_pixels > 0) {
        printf("Largest difference at position (%d, %d):\n", 
               largest_diff.x, largest_diff.y);
        printf("Image A pixel: R=%d G=%d B=%d A=%d\n",
               largest_diff.pixel_a.r, largest_diff.pixel_a.g,
               largest_diff.pixel_a.b, largest_diff.pixel_a.a);
        printf("Image B pixel: R=%d G=%d B=%d A=%d\n",
               largest_diff.pixel_b.r, largest_diff.pixel_b.g,
               largest_diff.pixel_b.b, largest_diff.pixel_b.a);
    }

    // Cleanup
    stbi_image_free(img1);
    stbi_image_free(img2);
    free(diff_img);
    return 0;
}