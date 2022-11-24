
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "types.hpp"
#include "sd.hpp"

extern "C" {
#include "miniz.h"
}

#include "./ChaN/ffconf.h"
#include "./ChaN/fileff.h"

bool loadBin( const std::vector<uint8_t> &file );

void initYAPFS(std::vector<uint8_t>& storage);
struct FileInfo : public YAPFS::FILINFO {
    const char *name() const {
        return lfsize ? lfname : fname;
    }
};

class File {
    YAPFS::FIL handle;
public:
    inline static int error;

    File(){
        handle.fs = nullptr;
    }

    File(const File&) = delete;

    File(File&& other) : handle(other.handle) {
        other.handle.fs = nullptr;
    }

    ~File(){
        close();
    }

    void close(){
        if(handle.fs){
            f_close(&handle);
            handle.fs = nullptr;
        }
    }

    operator bool(){
        return handle.fs;
    }

    static FileInfo stat(const char *name) {
        FileInfo info;
        error = f_stat(name, &info);
        return info;
    }

    File& openRO(const char *name) {
        close();
        error = f_open(&handle, name, FA_READ);
        return *this;
    }

    File& openRW(const char *name, bool create, bool append) {
        close();
        error = f_open(&handle, name, FA_READ|FA_WRITE|(create?FA_CREATE_ALWAYS:FA_OPEN_ALWAYS));
        if( !error && append){
            f_lseek(&handle, handle.fsize);
        }
        return *this;
    }

    uint32_t size(){
        return handle.fsize;
    }

    uint32_t tell(){
        return f_tell(&handle);
    }

    File& seek(uint32_t offset){
        f_lseek(&handle, offset);
        return *this;
    }

    uint32_t read( void *ptr, uint32_t count ){
        if(!*this) return 0;
        YAPFS::UINT n = 0;
        error = f_read(&handle, ptr, count, &n);
        return n;
    }

    uint32_t write( const void *ptr, uint32_t count ){
        if(!*this) return 0;
        uint32_t total = 0;
        YAPFS::UINT n = 0;
        error = f_write(&handle, ptr, count, &n);
        return n;
    }

    template< typename T, size_t S > uint32_t read( T (&data)[S] ){
	    return read( data, sizeof(data) );
    }

    template< typename T, size_t S > uint32_t write( const T (&data)[S] ){
	    return write( data, sizeof(data) );
    }

    template< typename T >
    T read(){
        T tmp = {};
        read( (void*) &tmp, sizeof(T) );
        return tmp;
    }

    template< typename T >
    File & operator >> (T &i){
    	read(&i, sizeof(T));
    	return *this;
    }

    template< typename T >
    File & operator << (const T& i){
    	write(&i, sizeof(T));
    	return *this;
    }

    File & operator << (const char *str ){
        uint32_t len = 0;
        while(str[len]) len++;
        write(str, len);
    	return *this;
    }
};

bool convertFile(const std::string& zipFileName) {
    mz_bool status;
    mz_zip_archive zip_archive;
    // Now try to open the archive.
    memset(&zip_archive, 0, sizeof(zip_archive));
    status = mz_zip_reader_init_file(&zip_archive, zipFileName.c_str(), 0);
    if (!status) {
        return false;
    }
    bool binLoaded = false;
    size_t accSize = 0;
    std::string fileName;
    auto numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            printf("mz_zip_reader_file_stat() failed!\n");
            continue;
        }

        fileName = file_stat.m_filename;
        if (fileName.find("__MACOSX") != std::string::npos)
            continue;
        if (fileName.find(".DS_Store") != std::string::npos)
            continue;

        auto size = (size_t)file_stat.m_uncomp_size;
        size = (size | 0x1FF) + 1;
        accSize += size;
    }

    accSize += 1024 * 1024; // at least 1mb for free space + book keeping
    accSize *= 2;           // error margin
    accSize |= 0xFFFFF;     // round up to nearest MB
    accSize++;
    // std::cout << "Initializing image " << accSize << std::endl;

    std::vector<uint8_t> image;
    image.resize(accSize);
    initYAPFS(image);

    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            printf("mz_zip_reader_file_stat() failed!\n");
            continue;
        }

        fileName = file_stat.m_filename;
        if (fileName.find("__MACOSX") != std::string::npos)
            continue;
        if (fileName.find(".DS_Store") != std::string::npos)
            continue;

        auto isDir = mz_zip_reader_is_file_a_directory(&zip_archive, i);
        if (isDir) {
            fileName = fileName.substr(0, fileName.size() - 1);
            if (auto error = YAPFS::f_mkdir(fileName.c_str())) {
                std::cout << "Error creating dir " << fileName << std::endl;
                continue;
            }
            continue;
        }

        size_t uncomp_size;
        auto p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
        if (!p) {
            printf("mz_zip_reader_extract_file_to_heap() failed!\n");
            continue;
        }

        if (!binLoaded) {
            if (auto it = fileName.rfind('.'); it != std::string::npos) {
                auto ext = fileName.substr(it + 1, std::string::npos);
                std::transform(ext.begin(), ext.end(), ext.begin(), [](char c){return std::tolower(c);});
                if (ext == "pop" || ext == "bin") {
                    binLoaded = loadBin({reinterpret_cast<uint8_t*>(p), reinterpret_cast<uint8_t*>(p) + uncomp_size});
                }
            }
        }

        File file;
        file.openRW(fileName.c_str(), true, false);
        file.write(p, uncomp_size);
        mz_free(p);
    }

    SD::length = image.size();
    SD::image = std::make_unique<u8[]>(image.size());
    memcpy(SD::image.get(), image.data(), image.size());

    mz_zip_reader_end(&zip_archive);
    return binLoaded;
}
