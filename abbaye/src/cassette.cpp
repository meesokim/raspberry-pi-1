#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cassette.h"
#include <bzlib.h>
#include <miniz_zip.h>

static int compare_zfiles(const void* a, const void* b) {
    return strcmp(((ZFile*)a)->fname, ((ZFile*)b)->fname);
}

static bool is_supported_extension(const char* ext) {
    char lower_ext[16];
    strncpy(lower_ext, ext, 15);
    lower_ext[15] = 0;
    for (int i = 0; lower_ext[i]; i++) {
        if (lower_ext[i] >= 'A' && lower_ext[i] <= 'Z') {
            lower_ext[i] += 32;
        }
    }
    return (strcmp(lower_ext, ".tap") == 0 ||
            strcmp(lower_ext, ".cas") == 0 ||
            strcmp(lower_ext, ".zip") == 0 ||
            strcmp(lower_ext, ".bz2") == 0);
}

#define PULSE 14
char Cassette::read(uint32_t cycles, uint8_t wait) {
    char val = 0;
    wait = 38;
    int diff = (int)(cycles - old_cycles);
    if ((uint32_t)diff > 4 * PULSE * 90)
    {
        mark = -3;
        inv_time = 0;
    } else if (mark < -2)
    {
        mark++;
    } else if (len && mark < 0)
    {
        mark = (tape[pos] == '1' ? 1:0);
        old_time = cycles;
        inv_time = old_time + 120;
        end_time = old_time + 1250 + PULSE * wait * mark;
        if (++pos > len)
        {
            pos = 0;
            printf("tape rewinded.\n");
        }
    }
    if (mark > -1)
    {
        if ((int)(cycles - inv_time) < 0)
            val = 0;
        else if ((int)(cycles - end_time) < 0)
            val = 1;
    }
    if ((int)(cycles - end_time) > 0)
        mark = -1;
    old_cycles = cycles;
    return val;
}

void Cassette::write(char ch)
{
}

void Cassette::load(const char *name) 
{
    pos = 0;
    len = 0;
    char file[256];
    int size = 0;
    if (!name)
    {
        if (num_files == 0)
        {
            printf("load: files is empty, nothing to load\n");
            return;
        }
        strncpy(file, files[file_index].fname, 255);
        file[255] = 0;
    } else {
        strncpy(file, name, 255);
        file[255] = 0;
    }        
    char *Buffer = new char[TAPE_SIZE];
#ifdef __circle__
    FIL File;
    unsigned int nBytesRead = 0;
    FRESULT Result = f_open (&File, file, FA_READ | FA_OPEN_EXISTING);
    if (Result == FR_OK) {
        f_read (&File, Buffer, TAPE_SIZE, &nBytesRead);
        f_close (&File);
    }
    size = nBytesRead;
#else        
    memset(tape, 0, sizeof tape);
    FILE *f = fopen(file, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        if (size > TAPE_SIZE) size = TAPE_SIZE;
        fseek(f, 0, SEEK_SET);
        size = fread(Buffer, 1, size, f);
        fclose(f);
    }
#endif
    strncpy(loaded_filename, file, 255);
    loaded_filename[255] = 0;
    
    const char *dot = strrchr(file, '.');
    char ext[16];
    if (dot) {
        strncpy(ext, dot, 15);
        ext[15] = 0;
        for (int i = 0; ext[i]; i++) {
            if (ext[i] >= 'A' && ext[i] <= 'Z') ext[i] += 32;
        }
    } else {
        ext[0] = 0;
    }

    if (strcmp(ext, ".bz2") == 0) 
    {
        bz_stream bStream;
        bStream.next_in = Buffer;
        bStream.avail_in = size;
        bStream.next_out = tape;
        bStream.avail_out = len;
        BZ2_bzDecompressInit(&bStream, 0, 0);
        int bReturn = BZ2_bzDecompress(&bStream);
    }
    else if (strcmp(ext, ".tap") == 0) 
    {
        memcpy(tape, Buffer, size);
        len = size;
    } 
    else if (strcmp(ext, ".cas") == 0) 
    {
        len = 0;
        for(int i = 0; i < size; i++)
        {
            uint8_t c = Buffer[i];
            for(int j = 0; j < 8; j++)  
            {
                tape[i*8+j] = (c&(0x80>>j))>0 ? '1' : '0';
                len++;
            }
        }
    } 
    else if (strcmp(ext, ".zip") == 0) 
    {
        len = loadzip(Buffer, size);
    }
    delete[] Buffer;
}

void Cassette::load(const char *data, int length, const char *filename)
{
    if (data[0] == 'P' && data[1] == 'K')
    {
        loadzip(data, length);
    }
    else
    {
        memset(tape, 0, sizeof tape);
        len = length > sizeof tape ? sizeof tape : length;
        memcpy(tape, data, len);
        strncpy(loaded_filename, filename, 255);
        loaded_filename[255] = 0;
    }
}

#include <sys/stat.h>
void Cassette::setfile(const char *filename)
{
    struct stat sb;
    if (stat(filename, &sb) != 0)
    {
        printf("setfile: %s not found\n", filename);
        return;
    }
    
    const char *dot = strrchr(filename, '.');
    bool is_zip = false;
    if (dot) {
        char lower_ext[16];
        strncpy(lower_ext, dot, 15);
        lower_ext[15] = 0;
        for (int i = 0; lower_ext[i]; i++) {
            if (lower_ext[i] >= 'A' && lower_ext[i] <= 'Z') lower_ext[i] += 32;
        }
        if (strcmp(lower_ext, ".zip") == 0) is_zip = true;
    }

    if (S_ISDIR(sb.st_mode))
    {
        loaddir(filename);
        printf("loaddir\n");
    }
    else if (is_zip)
    {
        loadzip(filename);
        printf("loadzip\n");
    }
    else {
        load(filename);
        printf("load\n");
    }
}

void Cassette::loaddir(const char *dirname)
{
    printf("loaddir:%s\n", dirname);
    num_files = 0;
#ifdef __circle__
    DIR Directory;
    FILINFO FileInfo;
    FRESULT Result = f_findfirst (&Directory, &FileInfo, dirname, "*");
    for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
    {
        const char *dot = strrchr(FileInfo.fname, '.');
        if (dot && is_supported_extension(dot))
        {
            if (num_files < MAX_FILES) {
                files[num_files++] = ZFile(FileInfo.fname);
            }
        }
        Result = f_findnext (&Directory, &FileInfo);
    }
#else
    #include <dirent.h>
    DIR *dir = opendir(dirname);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            const char *dot = strrchr(ent->d_name, '.');
            if (dot && is_supported_extension(dot)) {
                if (num_files < MAX_FILES) {
                    files[num_files++] = ZFile(ent->d_name);
                }
            }
        }
        closedir(dir);
    }
#endif
    qsort(files, num_files, sizeof(ZFile), compare_zfiles);
    file_index = 0;
    load();
}

int Cassette::loadzip(const char *data, int size)
{
    size_t uncomp_size, len; 
    mz_zip_archive zip;
    mz_zip_archive_file_stat file_stat;
    const size_t comp_buf_size = 1024*1024*1;
    const size_t uncomp_buf_size = 1024*1024*4;
    uint8_t *compresssed = new uint8_t[comp_buf_size];
    uint8_t *uncompressed = new uint8_t[uncomp_buf_size];
    char unzipfile[1024];
    memset(tape, 0, sizeof tape);
    memset(&zip, 0, sizeof(zip));
    len = 0;
    if (!size)
    {
        FILE *file = fopen(data, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            size = ftell(file);
            if (size > TAPE_SIZE) size = TAPE_SIZE;
            fseek(file, 0, SEEK_SET);
            size = fread(compresssed, 1, size, file);
            fclose(file);
            printf("loadzip: %s (%d)\n", data, size);
        } else {
            printf("loadzip: failed to open %s\n", data);
            size = 0;
        }
    }
    else 
    {
        memcpy(compresssed, data, size);
    }
    if (mz_zip_reader_init_mem(&zip, compresssed, size, 0))
    {
        for (mz_uint no = 0;no < mz_zip_reader_get_num_files(&zip); no++)
        {
            if (!mz_zip_reader_file_stat(&zip, no, &file_stat))
            {
                mz_zip_reader_end(&zip);
                break;
            }
            if (!strlen(file_stat.m_filename))
                continue;
            strcpy(unzipfile, file_stat.m_filename);
            uncomp_size = file_stat.m_uncomp_size;
            ZFile file(unzipfile);
            
            const char* ext_dot = file.extension();
            char ext[16];
            strncpy(ext, ext_dot, 15);
            ext[15] = 0;
            for (int i = 0; ext[i]; i++) {
                if (ext[i] >= 'A' && ext[i] <= 'Z') ext[i] += 32;
            }
            
            if (strcmp(ext, ".tap") == 0)
            {
                bool ret = mz_zip_reader_extract_file_to_mem(&zip, unzipfile, uncompressed, uncomp_buf_size, 0);
                if (!ret)
                {
                    printf("fatal error\n");
                    delete[] compresssed;
                    delete[] uncompressed;
                    exit(0);
                }
                memcpy(tape+len, uncompressed, uncomp_size);
                len += uncomp_size;
            } 
            else if (strcmp(ext, ".cas") == 0)
            {
                bool ret = mz_zip_reader_extract_file_to_mem(&zip, unzipfile, uncompressed, uncomp_buf_size, 0);
                if (!ret)
                {
                    printf("fatal error\n");
                    delete[] compresssed;
                    delete[] uncompressed;
                    exit(0);
                }
                for(int i = 0; i < uncomp_size; i++)
                {
                    for(int j = 0; j < 8; j++)  
                    {
                        tape[len+i*8+j] = (uncompressed[i]&(0x80>>j))>0 ? '1' : '0';
                    }
                }
                uncomp_size = uncomp_size * 8; 
                len += uncomp_size;
            } else 
                continue;
            printf("unzip:%d. %s(%d)\n", no+1, unzipfile, (int) uncomp_size);
            if (!no) {
                strncpy(loaded_filename, unzipfile, 255);
                loaded_filename[255] = 0;
            }
        }
    }
    mz_zip_reader_end(&zip);
    delete[] compresssed;
    delete[] uncompressed;
    return len;
}
