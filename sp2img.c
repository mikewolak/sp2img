/***************************************************************************
*  File    : sp2img.c                                                      *
*  Purpose : Convert Peavey SP sampler .sp files to .img format for GW     *
*  Language: C99                                                           *
*  Usage   : sp2img input.sp output.img                                    *
*  Comment : .sp format: T0H0,T0H1,T1H0,T1H1... -> .img: all H0, then H1  *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define SECTORS_PER_TRACK   18
#define BYTES_PER_SECTOR    512
#define TRACKS              80
#define HEADS               2
#define TRACK_SIZE          (SECTORS_PER_TRACK * BYTES_PER_SECTOR)  /* 9216 */
#define EXPECTED_SIZE       (TRACKS * HEADS * TRACK_SIZE)           /* 1474560 */

static void print_usage(const char *progname) {
    printf("SP to IMG Converter v1.0\n");
    printf("Converts Peavey SP sampler .sp files to standard .img format\n");
    printf("Usage: %s input.sp output.img\n\n", progname);
    printf("File format conversion:\n");
    printf("  .sp format: Track 0 Head 0, Track 0 Head 1, Track 1 Head 0, etc.\n");
    printf("  .img format: All Head 0 tracks, then all Head 1 tracks\n");
    printf("  Expected size: %d bytes (%.1f MB)\n", EXPECTED_SIZE, EXPECTED_SIZE/1024.0/1024.0);
}

static long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static int convert_sp_to_img(const char *sp_filename, const char *img_filename) {
    FILE *sp_file = NULL;
    FILE *img_file = NULL;
    unsigned char *track_buffer = NULL;
    int result = 0;
    
    // Verify input file exists and has correct size
    long file_size = get_file_size(sp_filename);
    if (file_size == -1) {
        fprintf(stderr, "Error: Cannot access %s\n", sp_filename);
        return 1;
    }
    
    if (file_size != EXPECTED_SIZE) {
        fprintf(stderr, "Warning: %s is %ld bytes, expected %d\n", 
                sp_filename, file_size, EXPECTED_SIZE);
        if (file_size < EXPECTED_SIZE) {
            fprintf(stderr, "Error: File too small, aborting\n");
            return 1;
        }
    }
    
    // Allocate buffer for one track (9216 bytes)
    track_buffer = malloc(TRACK_SIZE);
    if (!track_buffer) {
        fprintf(stderr, "Error: Cannot allocate %d bytes for track buffer\n", TRACK_SIZE);
        return 1;
    }
    
    // Open files
    sp_file = fopen(sp_filename, "rb");
    if (!sp_file) {
        fprintf(stderr, "Error: Cannot open %s for reading: %s\n", 
                sp_filename, strerror(errno));
        result = 1;
        goto cleanup;
    }
    
    img_file = fopen(img_filename, "wb");
    if (!img_file) {
        fprintf(stderr, "Error: Cannot create %s: %s\n", 
                img_filename, strerror(errno));
        result = 1;
        goto cleanup;
    }
    
    printf("Converting %s -> %s\n", sp_filename, img_filename);
    printf("Processing tracks: ");
    fflush(stdout);
    
    // Phase 1: Write all Head 0 tracks
    for (int track = 0; track < TRACKS; track++) {
        if (track % 20 == 0) {
            printf("%d", track);
            fflush(stdout);
        } else if (track % 5 == 0) {
            printf(".");
            fflush(stdout);
        }
        
        // Seek to Head 0 data for this track in .sp file
        // .sp layout: T0H0, T0H1, T1H0, T1H1, T2H0, T2H1, ...
        long sp_offset = (long)track * 2 * TRACK_SIZE;
        if (fseek(sp_file, sp_offset, SEEK_SET) != 0) {
            fprintf(stderr, "\nError: Cannot seek to track %d head 0 in %s\n", 
                    track, sp_filename);
            result = 1;
            goto cleanup;
        }
        
        // Read Head 0 track data
        size_t bytes_read = fread(track_buffer, 1, TRACK_SIZE, sp_file);
        if (bytes_read != TRACK_SIZE) {
            fprintf(stderr, "\nError: Cannot read track %d head 0 (got %zu bytes)\n", 
                    track, bytes_read);
            result = 1;
            goto cleanup;
        }
        
        // Write to .img file
        size_t bytes_written = fwrite(track_buffer, 1, TRACK_SIZE, img_file);
        if (bytes_written != TRACK_SIZE) {
            fprintf(stderr, "\nError: Cannot write track %d head 0 to %s\n", 
                    track, img_filename);
            result = 1;
            goto cleanup;
        }
    }
    
    printf(" | ");
    fflush(stdout);
    
    // Phase 2: Write all Head 1 tracks  
    for (int track = 0; track < TRACKS; track++) {
        if (track % 20 == 0) {
            printf("%d", track);
            fflush(stdout);
        } else if (track % 5 == 0) {
            printf(".");
            fflush(stdout);
        }
        
        // Seek to Head 1 data for this track in .sp file
        long sp_offset = (long)track * 2 * TRACK_SIZE + TRACK_SIZE;
        if (fseek(sp_file, sp_offset, SEEK_SET) != 0) {
            fprintf(stderr, "\nError: Cannot seek to track %d head 1 in %s\n", 
                    track, sp_filename);
            result = 1;
            goto cleanup;
        }
        
        // Read Head 1 track data
        size_t bytes_read = fread(track_buffer, 1, TRACK_SIZE, sp_file);
        if (bytes_read != TRACK_SIZE) {
            fprintf(stderr, "\nError: Cannot read track %d head 1 (got %zu bytes)\n", 
                    track, bytes_read);
            result = 1;
            goto cleanup;
        }
        
        // Write to .img file
        size_t bytes_written = fwrite(track_buffer, 1, TRACK_SIZE, img_file);
        if (bytes_written != TRACK_SIZE) {
            fprintf(stderr, "\nError: Cannot write track %d head 1 to %s\n", 
                    track, img_filename);
            result = 1;
            goto cleanup;
        }
    }
    
    printf("\nSuccess! Created %s (%d bytes)\n", img_filename, EXPECTED_SIZE);
    printf("Use with GreaseWeazle: gw write --format=ibm.1440 %s\n", img_filename);

cleanup:
    if (track_buffer) free(track_buffer);
    if (sp_file) fclose(sp_file);
    if (img_file) fclose(img_file);
    
    // Clean up output file on error
    if (result != 0 && img_filename) {
        remove(img_filename);
    }
    
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *sp_file = argv[1];
    const char *img_file = argv[2];
    
    // Basic input validation
    if (strlen(sp_file) == 0 || strlen(img_file) == 0) {
        fprintf(stderr, "Error: Empty filename provided\n");
        return 1;
    }
    
    // Check if output file already exists
    if (get_file_size(img_file) >= 0) {
        printf("Warning: %s already exists. Overwrite? (y/n): ", img_file);
        int c = getchar();
        if (c != 'y' && c != 'Y') {
            printf("Aborted.\n");
            return 1;
        }
    }
    
    int result = convert_sp_to_img(sp_file, img_file);
    
    if (result == 0) {
        printf("\nConversion completed successfully!\n");
    } else {
        printf("Conversion failed!\n");
    }
    
    return result;
}

/**************************************************************************/
