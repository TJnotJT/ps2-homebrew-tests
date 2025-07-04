#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <tamtypes.h>

#pragma pack(push, 1) // Ensure structures are tightly packed

// Bitmap File Header
typedef struct {
    uint16_t bfType;      // "BM" signature
    uint32_t bfSize;      // File size in bytes
    uint16_t bfReserved1; // Reserved (set to 0)
    uint16_t bfReserved2; // Reserved (set to 0)
    uint32_t bfOffBits;   // Offset to pixel data
} BITMAPFILEHEADER;

// Bitmap Info Header
typedef struct {
    uint32_t biSize;          // Size of the DIB header (40 bytes for BITMAPINFOHEADER)
    int32_t  biWidth;         // Image width in pixels
    int32_t  biHeight;        // Image height in pixels
    uint16_t biPlanes;        // Number of color planes (must be 1)
    uint16_t biBitCount;      // Bits per pixel (e.g., 24 for 24-bit color)
    uint32_t biCompression;   // Compression method (0 for uncompressed)
    uint32_t biSizeImage;     // Image size in bytes (can be 0 for uncompressed)
    int32_t  biXPelsPerMeter; // Horizontal resolution (pixels per meter)
    int32_t  biYPelsPerMeter; // Vertical resolution (pixels per meter)
    uint32_t biClrUsed;       // Number of colors in the color palette (0 for default)
    uint32_t biClrImportant;  // Number of important colors (0 for all)
} BITMAPINFOHEADER;

#pragma pack(pop)

int write_bmp(const char* filename, const u8* pixelData, int width, int height, int bitsPerPixel)
{
    const int bytesPerPixel = bitsPerPixel / 8;

    u8* pixelDataBGR = (u8*)aligned_alloc(64, width * height * bytesPerPixel);

    if (!pixelDataBGR) {
        printf("Memory allocation failed\n");
        return -1;
    }

    // Convert to BGR format
    for (int i = 0; i < width * height; i++) {
        if (bitsPerPixel == 24) {
            pixelDataBGR[i * 3 + 0] = pixelData[i * 3 + 2]; // B
            pixelDataBGR[i * 3 + 1] = pixelData[i * 3 + 1]; // G
            pixelDataBGR[i * 3 + 2] = pixelData[i * 3 + 0]; // R
        } else if (bitsPerPixel == 32) {
            pixelDataBGR[i * 4 + 0] = pixelData[i * 4 + 2]; // B
            pixelDataBGR[i * 4 + 1] = pixelData[i * 4 + 1]; // G
            pixelDataBGR[i * 4 + 2] = pixelData[i * 4 + 0]; // R
            pixelDataBGR[i * 4 + 3] = pixelData[i * 4 + 3]; // A (Alpha, if present)
        } else {
            printf("Unsupported bits per pixel: %d\n", bitsPerPixel);
            free(pixelDataBGR);
            return -1;
        }
    }

    // Calculate row padding
    const int rowSizeWithoutPadding = width * bytesPerPixel;
    const int paddedRowSize = (rowSizeWithoutPadding + 3) & (~3); // Ensure multiple of 4 bytes

    const int pixelDataSize = paddedRowSize * height;
    const int fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pixelDataSize;

    // Prepare headers
    BITMAPFILEHEADER fileHeader;
    fileHeader.bfType = 0x4D42; // "BM" in ASCII, little-endian
    fileHeader.bfSize = fileSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = bitsPerPixel;
    infoHeader.biCompression = 0; // No compression
    infoHeader.biSizeImage = pixelDataSize;
    infoHeader.biXPelsPerMeter = 0; // Not critical for basic BMP
    infoHeader.biYPelsPerMeter = 0; // Not critical for basic BMP
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    // Open file for writing
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error opening file for writing\n");
        return -1;
    }

    // Write headers
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    unsigned char paddingBytes[3] = {0, 0, 0}; // 3 bytes of padding

    // Write pixel data (with padding)
    // Note: BMP files store pixel data from bottom to top
    for (int y = height - 1; y >= 0; y--) {

      if (fwrite(pixelDataBGR + y * rowSizeWithoutPadding, 1, rowSizeWithoutPadding, fp) != rowSizeWithoutPadding) {
        printf("Error writing pixel data\n");
        fclose(fp);
        return -1;
      }
      // Write padding bytes if necessary
      int padding = paddedRowSize - rowSizeWithoutPadding;
      if (padding > 0) {
          fwrite(paddingBytes, 1, padding, fp);
      }
    }

    // Clean up
    fclose(fp);

    printf("BMP file created successfully!\n");

    return 0;
}