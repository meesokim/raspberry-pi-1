#ifndef CASSETTE_H_
#define CASSETTE_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __circle__
#include <fatfs/ff.h>
#endif

enum casmode {CASSETTE_STOP, CASSETTE_PLAY, CASSETTE_REC};
enum castype {TYPE_CHARBIN, TYPE_BINARY};

class ZFile {
    public:
    char fname[256];
    int index;
    ZFile() { fname[0] = 0; index = 0; }
    ZFile(const char* f, int i=0) {
        strncpy(fname, f, 255);
        fname[255] = 0;
        index = i;
    }
    bool operator<(const ZFile& other) const {
        return strcmp(fname, other.fname) < 0;
    }
    const char *c_str() { return fname; }
    const char *filename() { return fname; }
    const char *extension() {
        const char *dot = strrchr(fname, '.');
        return dot ? dot : "";
    }
};

#define MAX_FILES 512
#define TAPE_SIZE (1024 * 1024 * 6)

class Cassette {
    uint32_t old_cycles;
    char tape[TAPE_SIZE];
    int len = 0;
    char type = TYPE_CHARBIN;
    char mark = -1;
    uint32_t inv_time, end_time, old_time;
    
    ZFile files[MAX_FILES];
    int num_files = 0;
    int file_index = 0;
    char *dirname;
    char loaded_filename[256];
    const char* exts[4] = {".tap",".cas",".zip",".bz2"};
public:
    char motor;
    int pos = 0;
    Cassette() { loaded_filename[0] = 0; dirname = NULL; }
    void initTick(uint32_t tick) { old_cycles = tick; }
    void load(const char *name = NULL);
    void load(const char *data, int length, const char *filename);
    void save(const char *name);
    char read(uint32_t, uint8_t);
    char read1() { return 0;}
    void write(char);
    void next() { if (num_files == 0) return; if (++file_index >= num_files) file_index = 0; load(); }
    void get_title(char *buf) { if (loaded_filename[0] == 0) { buf[0] = 0; } else { strcpy(buf, loaded_filename); } };
    void prev() { if (num_files == 0) return; if (--file_index < 0) file_index = num_files - 1; load();}
    void settape(unsigned int i) 
    {
        if (num_files == 0) return;
        if ( i >= (unsigned int)num_files )
            file_index = num_files - 1; 
        else 
            file_index = i; 
        load(); 
    };
    void setfile(const char *);
    void loaddir(const char *);
    int loadzip(const char *, int len = 0);
};

#endif