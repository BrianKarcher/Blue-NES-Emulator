#include "INESLoader.h"
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <fstream>

ines_file_t ines_file;

// Function to validate iNES header
bool INESLoader::validate_ines_header(const ines_header_t* header) {
    return (header->signature[0] == 'N' &&
        header->signature[1] == 'E' &&
        header->signature[2] == 'S' &&
        header->signature[3] == 0x1A);
}

// Function to load CHR-ROM data from iNES file
ines_file_t* INESLoader::load_data_from_ines(const wchar_t* filename) {
    FILE* file = nullptr;
    errno_t err = _wfopen_s(&file, filename, L"rb");
    
    if (err != 0 || file == nullptr) {
        MessageBoxA(NULL, "The specified iNES file does not exist or cannot be accessed.", "Error", MB_OK | MB_ICONERROR);
        return nullptr;
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

    // Read PRG-ROM data
    prg_data->data = (uint8_t*)malloc(prg_rom_size);
    if (fread(prg_data->data, prg_rom_size, 1, file) != 1) {
		MessageBoxA(NULL, "Failed to read PRG-ROM data from the file.", "Error", MB_OK | MB_ICONERROR);
        printf("Error: Cannot read PRG-ROM data\n");
        free(prg_data->data);
        free(prg_data);
        fclose(file);
        return NULL;
    }
    prg_data->size = prg_rom_size;

    // Allocate memory for CHR-ROM data
    chr_rom_data_t* chr_data = (chr_rom_data_t*)malloc(sizeof(chr_rom_data_t));
	ines_file.chr_rom = chr_data;
    if (!chr_data) {
		MessageBoxA(NULL, "Failed to allocate memory for CHR data structure.", "Error", MB_OK | MB_ICONERROR);
        printf("Error: Cannot allocate memory for CHR data structure\n");
        fclose(file);
        return NULL;
    }

    if (chr_rom_size == 0) {
        // CHR-RAM
        chr_data->data = nullptr;
    }
    else {
        chr_data->data = (uint8_t*)malloc(chr_rom_size);
        if (!chr_data->data) {
            MessageBoxA(NULL, "Failed to allocate memory for CHR-ROM data.", "Error", MB_OK | MB_ICONERROR);
            printf("Error: Cannot allocate memory for CHR-ROM data\n");
            free(chr_data);
            fclose(file);
            return NULL;
        }

        // Read CHR-ROM data
        if (fread(chr_data->data, chr_rom_size, 1, file) != 1) {
            MessageBoxA(NULL, "Failed to read CHR-ROM data from the file.", "Error", MB_OK | MB_ICONERROR);
            printf("Error: Cannot read CHR-ROM data\n");
            free(chr_data->data);
            free(chr_data);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);

    // Set up structure
    chr_data->size = chr_rom_size;

    // Calculate bitmap dimensions (16x16 tiles for 8KB, 32x32 for 16KB, etc.)
    int num_tiles = chr_rom_size / 16;  // Each tile is 16 bytes
    int tiles_per_row = 16;  // Arrange in 16-tile rows
    int num_rows = (num_tiles + tiles_per_row - 1) / tiles_per_row;

    printf("Successfully loaded CHR-ROM: %zu bytes\n",
        chr_rom_size);

    return &ines_file;
}