#include "ines.h"
#include <cstdlib>
#include <stdio.h>
#include <string.h>

// Function to validate iNES header
bool ines::validate_ines_header(const ines_header_t* header) {
    return (header->signature[0] == 'N' &&
        header->signature[1] == 'E' &&
        header->signature[2] == 'S' &&
        header->signature[3] == 0x1A);
}

// Function to load CHR-ROM data from iNES file
ines_file_t* ines::load_data_from_ines(const char* filename) {
	ines_file_t ines_file;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }

    // Read and validate header
    ines_header_t header;
    if (fread(&header, sizeof(ines_header_t), 1, file) != 1) {
        printf("Error: Cannot read iNES header\n");
        fclose(file);
        return NULL;
    }

    if (!validate_ines_header(&header)) {
        printf("Error: Invalid iNES header\n");
        fclose(file);
        return NULL;
    }
	ines_file.header = &header;

    // Check if CHR-ROM exists
    if (header.chr_rom_size == 0) {
        printf("Error: No CHR-ROM data in file\n");
        fclose(file);
        return NULL;
    }

    // Calculate sizes
    size_t prg_rom_size = header.prg_rom_size * 16384;  // 16KB units
    size_t chr_rom_size = header.chr_rom_size * 8192;   // 8KB units
    size_t trainer_size = (header.flags6 & 0x04) ? 512 : 0;

    // Skip trainer if present
    if (trainer_size > 0) {
        fseek(file, trainer_size, SEEK_CUR);
    }

    // Allocate memory for PRG-ROM data
    prg_rom_data_t* prg_data = (prg_rom_data_t*)malloc(sizeof(prg_rom_data_t));
    ines_file.prg_rom = prg_data;
    if (!prg_data) {
        printf("Error: Cannot allocate memory for PRG data structure\n");
        fclose(file);
        return NULL;
    }

    // Read CHR-ROM data
    if (fread(prg_data->data, chr_rom_size, 1, file) != 1) {
        printf("Error: Cannot read PRG-ROM data\n");
        free(prg_data->data);
        free(prg_data);
        fclose(file);
        return NULL;
    }

    prg_data->data = (uint8_t*)malloc(chr_rom_size);
    if (!prg_data->data) {
        printf("Error: Cannot allocate memory for CHR-ROM data\n");
        free(prg_data);
        fclose(file);
        return NULL;
    }

    // Read PRG-ROM data
    if (fread(prg_data->data, chr_rom_size, 1, file) != 1) {
        printf("Error: Cannot read CHR-ROM data\n");
        free(prg_data->data);
        free(prg_data);
        fclose(file);
        return NULL;
    }

    // Allocate memory for CHR-ROM data
    chr_rom_data_t* chr_data = (chr_rom_data_t*)malloc(sizeof(chr_rom_data_t));
	ines_file.chr_rom = chr_data;
    if (!chr_data) {
        printf("Error: Cannot allocate memory for CHR data structure\n");
        fclose(file);
        return NULL;
    }

    chr_data->data = (uint8_t*)malloc(chr_rom_size);
    if (!chr_data->data) {
        printf("Error: Cannot allocate memory for CHR-ROM data\n");
        free(chr_data);
        fclose(file);
        return NULL;
    }

    // Read CHR-ROM data
    if (fread(chr_data->data, chr_rom_size, 1, file) != 1) {
        printf("Error: Cannot read CHR-ROM data\n");
        free(chr_data->data);
        free(chr_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    // Set up structure
    chr_data->size = chr_rom_size;

    // Calculate bitmap dimensions (16x16 tiles for 8KB, 32x32 for 16KB, etc.)
    int num_tiles = chr_rom_size / 16;  // Each tile is 16 bytes
    int tiles_per_row = 16;  // Arrange in 16-tile rows
    int num_rows = (num_tiles + tiles_per_row - 1) / tiles_per_row;

    chr_data->width = tiles_per_row * 8;
    chr_data->height = num_rows * 8;

    // Allocate bitmap data
    size_t bitmap_size = chr_data->width * chr_data->height * sizeof(uint32_t);
    chr_data->bitmap_data = (uint32_t*)malloc(bitmap_size);
    if (!chr_data->bitmap_data) {
        printf("Error: Cannot allocate memory for bitmap data\n");
        free(chr_data->data);
        free(chr_data);
        return NULL;
    }

    // Convert CHR-ROM to bitmap
    memset(chr_data->bitmap_data, 0, bitmap_size);

    for (int i = 0; i < num_tiles; i++) {
        int tile_x = i % tiles_per_row;
        int tile_y = i / tiles_per_row;

        convert_chr_tile_to_bitmap(&chr_data->data[i * 16],
            chr_data->bitmap_data,
            tile_x, tile_y,
            chr_data->width, 0);
    }

    printf("Successfully loaded CHR-ROM: %zu bytes, %dx%d bitmap\n",
        chr_rom_size, chr_data->width, chr_data->height);

    return chr_data;
}