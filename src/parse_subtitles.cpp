#include "parse_subtitles.h"
// #include "io_Platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

struct file {
    uint8_t* data;
    int size;
};

subtitles parse_subtitles(uint8_t* data, int size);

FILE* open_filea(char* filename)
{
    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) {
        printf("Failed to load %s\n", filename);
        return NULL;
    }
    return fileptr;
}

file load_sub_file(char* path)
{
    file out = {0};
    if (path == nullptr) {
        out.data = nullptr;
        return out;
    }

    // create a path for the .sve counterpart to the .mve file being loaded
    // assumes subtitle file is in the same folder as the .mve file
    // TODO: subs might not be located next to .mve file normally?
    char buff[4096] = {0};
    int path_len = strlen(path);
    if (path_len > 4096) {
        printf("ERROR: Path buffer not big enough. Have: 4096 - Need: %d\n", path_len);
        out.data = nullptr;
        return out;
    }
    strncpy(buff, path, path_len);
    char* ptr = strrchr(buff, '.');
    if (ptr == nullptr) {
        printf("ERROR: Unable to find file extension: %s\n", path);
        out.data = nullptr;
        return out;
    }
    strncpy(&ptr[1], "sve", 4);


    FILE* subs_file = open_filea(buff);
    if (subs_file == nullptr) {
        printf("No matching subtitle file found.\n");
        out.data = nullptr;
        return out;
    }

    fseek(subs_file, 0, SEEK_END);
    int file_size = ftell(subs_file);
    uint8_t* data = (uint8_t*)malloc(file_size);
    if (data == nullptr) {
        printf("ERROR: Unable to allocate memory for subtitles: %s\n", buff);
    }
    fseek(subs_file, 0, SEEK_SET);

    fread(data, file_size, 1, subs_file);

    fclose(subs_file);

    out.data = data;
    out.size = file_size;

    return (out);
}

subtitles load_subs(char* path)
{
    file sub_file = load_sub_file(path);

    subtitles subs;
    if (sub_file.data == nullptr) {
        return subs;
    }
    subs = parse_subtitles(sub_file.data, sub_file.size);

    return subs;
}

subtitles parse_subtitles(uint8_t* data, int size)
{
    subtitles out  = {0};
    if (size == 0) {
        return out;
    }

    int count = 1;
    for (int i = 0; i < size; i++) {
        // get the number of lines in this text file
        if (data[i] == '\n') {
            count++;
        }
    }
    subtitle* subs = (subtitle*)malloc(sizeof(subtitle)*count);
    out.count      = count;
    out.data       = data;
    out.sub_list   = subs;
    out.data_size  = size;

    int line = 0;
    for (int i = 0; i < size; i++)
    {
        if (line > count) {
            printf("WTF is happening? There shouldn't be more lines than /n's, so wtf man?\n"
                    "lines: %d   count: %d\n", line, count);
            return out;
        }
        if (data[i] == '\r') {
            continue;
        }
        if (data[i] == '\n') {
            // subtitle line ended, start the next one
            line++;
            continue;
        }
        subs[line].frame_start = strtol((char*)&data[i], NULL, 10);

        while (i < size) {
            if (data[i] == ':') {
                // number ended, start of subtitle line is next character
                break;
            }
            if (data[i] == '\n') {
                // not sure what would be going on here, maybe an accidental blank subtitle line? missing ':'?
                break;
            }
            i++;
        }

        if (data[i] == ':') {
            i++;
            // assign subtitle pointer for this line
            subs[line].subtitle = (char*)&data[i];

            int start = i;      // keep track of starting index for this subtitle line
            while (i < size) {
                if (data[i] == '\n') {
                    break;
                }
                i++;
            }
            // get the length of this subtitle line
            subs[line].length = i - start -1;
            line++;
        }
    }

    printf("Subtitles successfully parsed");
    return out;
}