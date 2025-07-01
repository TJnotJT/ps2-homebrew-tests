#include <stdio.h>
#include <stdlib.h> // For malloc and free

// Function to write a PPM P6 file from raw RGB byte data
int write_ppm_p6(const char *filename, unsigned char *pixel_data, int width, int height) {
    FILE *fp;

    // Open the file in binary write mode
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Error opening file for writing");
        return 0; // Indicate failure
    }

    // Write the PPM header
    // P6: Magic number for binary color PPM
    // width height: Image dimensions
    // 255: Maximum color value (for 8-bit per channel)
    fprintf(fp, "P6\n%d %d\n255\n", width, height);

    // Write the raw pixel data
    // Each pixel is represented by 3 bytes (R, G, B)
    // The data is typically stored row by row, pixel by pixel (RGBRGB...)
    size_t bytes_written = fwrite(pixel_data, 1, width * height * 3, fp);

    if (bytes_written != (size_t)(width * height * 3)) {
        perror("Error writing pixel data");
        fclose(fp);
        return 0; // Indicate failure
    }

    // Close the file
    fclose(fp);
    return 1; // Indicate success
}

int main() {
    int width = 200;
    int height = 150;

    // Allocate memory for raw pixel data (RGB, 3 bytes per pixel)
    // Example: create a simple red rectangle
    unsigned char *image_data = (unsigned char *)malloc(width * height * 3);
    if (image_data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Fill the image data (e.g., a red image)
    for (int i = 0; i < width * height; ++i) {
        image_data[i * 3 + 0] = 255; // Red
        image_data[i * 3 + 1] = 0;   // Green
        image_data[i * 3 + 2] = 0;   // Blue
    }

    // Write the image data to a PPM file
    if (write_ppm_p6("output.ppm", image_data, width, height)) {
        printf("PPM file 'output.ppm' created successfully.\n");
    } else {
        printf("Failed to create PPM file.\n");
    }

    // Free the allocated memory
    free(image_data);

    return 0;
}