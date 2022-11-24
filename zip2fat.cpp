
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

#include "types.hpp"
#include "sd.hpp"

#include <archive.h>
#include <archive_entry.h>

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
    bool binLoaded = false;

    auto a = archive_read_new();
    archive_read_support_format_7zip(a);
    archive_read_support_format_gnutar(a);
    archive_read_support_format_rar(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);

    auto r = archive_read_open_filename(a, zipFileName.c_str(), 10240);
    if (r) {
        exit(1);
        return false;
    }

    struct FileEntry {
        std::string path;
        std::vector<u8> data;
        bool isDirectory;
    };
    std::vector<std::unique_ptr<FileEntry>> files;
    size_t totalSize = 0;

    for (;;) {
        archive_entry *entry = nullptr;
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK)
            exit(2);

        std::string fileName = archive_entry_pathname(entry);

        std::unique_ptr<FileEntry> fe;
        if (fileName.find("__MACOSX") != std::string::npos);
        else if (fileName.find(".DS_Store") != std::string::npos);
        else {
            fe = std::make_unique<FileEntry>();
            fe->path = fileName;
            fe->isDirectory = archive_entry_filetype(entry) == 16384;
        }

	for (;;) {
            const void *buff;
            size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
            int64_t offset;
#else
            off_t offset;
#endif
            r = archive_read_data_block(a, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
                break;
            if (r != ARCHIVE_OK)
                exit(4);
            if (!fe)
                continue;
            auto ubuff = reinterpret_cast<const u8*>(buff);
            fe->data.insert(fe->data.end(), ubuff, &ubuff[size]);
	}

        if (fe) {
            totalSize += (fe->data.size() | 0x1FF) + 1;
            files.push_back(std::move(fe));
        }
    }

    archive_read_close(a);
    archive_read_free(a);

    totalSize += 1024 * 1024; // at least 1mb for free space + book keeping
    totalSize *= 2;           // error margin
    totalSize |= 0xFFFFF;     // round up to nearest MB
    totalSize++;

    std::vector<uint8_t> image;
    image.resize(totalSize);
    initYAPFS(image);

    for (auto& entry : files) {
        auto fileName = entry->path;
        if (entry->isDirectory) {
            fileName = fileName.substr(0, fileName.size() - 1);
            if (auto error = YAPFS::f_mkdir(fileName.c_str())) {
                std::cout << "Error creating dir " << fileName << std::endl;
                continue;
            }
            continue;
        }

        if (!binLoaded) {
            if (auto it = fileName.rfind('.'); it != std::string::npos) {
                auto ext = fileName.substr(it + 1, std::string::npos);
                std::transform(ext.begin(), ext.end(), ext.begin(), [](char c){return std::tolower(c);});
                if (ext == "pop" || ext == "bin") {
                    binLoaded = loadBin(entry->data);
                }
            }
        }

        File file;
        file.openRW(fileName.c_str(), true, false);
        file.write(entry->data.data(), entry->data.size());
    }

    SD::length = image.size();
    SD::image = std::make_unique<u8[]>(image.size());
    memcpy(SD::image.get(), image.data(), image.size());

    return binLoaded;
}
