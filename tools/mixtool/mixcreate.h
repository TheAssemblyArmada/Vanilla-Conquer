// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef MIXCREATE_H
#define MIXCREATE_H

#include "crc.h"
#include "debugstring.h"
#include "endianness.h"
#include "xpipe.h"
#include "search.h"
#include "listnode.h"
#include "mixfile.h"
#include "mixnamedb.h"
#include "pk.h"
#include "pkpipe.h"
#include "shapipe.h"
#include "rndstraw.h"
#include <string.h>
#include <libgen.h>
#include <strings.h>
#include <algorithm>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

extern RandomStraw CryptRandom;
extern PKey PrivateKey;

// From mixfile.h
#pragma pack(push, 4)
struct SubBlock
{
    int32_t CRC;    // CRC code for embedded file.
    int32_t Offset; // Offset from start of data section.
    int32_t Size;   // Size of data subfile.

    int operator<(SubBlock& two) const
    {
        return (CRC < two.CRC);
    };
    int operator>(SubBlock& two) const
    {
        return (CRC > two.CRC);
    };
    int operator==(SubBlock& two) const
    {
        return (CRC == two.CRC);
    };
};
#pragma pack(pop)

#pragma pack(push, 2)
struct FileHeader
{
    int16_t count;
    int32_t size;
};
#pragma pack(pop)

template <typename TFC, typename TCRC> class MixFileCreatorClass
{
    enum MixFlags
    {
        HAS_CHECKSUM = 0x00010000,
        IS_ENCRYPTED = 0x00020000
    };

private:
    class FileDataNode : public VanillaNode<FileDataNode>
    {
    public:
        FileDataNode()
            : VanillaNode<FileDataNode>()
            , Entry()
            , FilePath(nullptr)
            , InMix(false)
        {
        }
        ~FileDataNode();

    public:
        SubBlock Entry;
        char* FilePath;
        int Padding;
        bool InMix;
    };

public:
    MixFileCreatorClass(char const* filename,
                        unsigned alignment = 1,
                        bool has_checksum = false,
                        bool is_encrypted = false,
                        bool quiet = true,
                        bool force_flags = false,
                        MixNameDatabase* name_db = nullptr);
    ~MixFileCreatorClass();

    char const* Get_Filename() const
    {
        char const* name = NewMixHandle->File_Name();
        if (name == nullptr) {
            return TempFilename;
        }
        return name;
    }

    char const* Get_Temp_Filename() const
    {
        return TempFilename;
    };
    void Set_Filename(char const* filename)
    {
        NewMixHandle->Set_Name(filename);
    }
    void Add_File(char const* filename);
    void Write_Mix();

private:
    // This helper is used for alignment.
    // If an alignment is passed that is not a power of two, the next largest power of 2 will be returned.
    unsigned Power_Of_Two(unsigned val)
    {
        if (val == 0) {
            return 1;
        }

        if ((val & (val - 1)) == 0) {
            return val;
        }

        unsigned power = 1;

        while (power < val) {
            power *= 2;
        }

        return power;
    }

    unsigned To_Pad(unsigned val)
    {
        unsigned remainder = val % Alignment;

        return remainder == 0 ? 0 : Alignment - remainder;
    }

    static char TempFilename[];

protected:
    VanillaList<FileDataNode> FileList;
    IndexClass<FileDataNode*> FileIndex;
    TFC* NewMixHandle;
    MixNameDatabase* NameDatabase;
    unsigned Alignment;
    bool HasChecksum;
    bool IsEncrypted;
    bool ForceFlags;
    bool Quiet;
    FileHeader Header;

private:
    int Write_File(char const* filename, Pipe* pipe);
};

template <typename TFC, typename TCRC> char MixFileCreatorClass<TFC, TCRC>::TempFilename[] = "makemix.mix";

template <typename TFC, typename TCRC> MixFileCreatorClass<TFC, TCRC>::FileDataNode::~FileDataNode()
{
    if (FilePath) {
        free(FilePath);
    }
    FilePath = nullptr;
}

template <typename TFC, typename TCRC>
MixFileCreatorClass<TFC, TCRC>::MixFileCreatorClass(char const* filename,
                                                    unsigned alignment,
                                                    bool has_checksum,
                                                    bool is_encrypted,
                                                    bool quiet,
                                                    bool force_flags,
                                                    MixNameDatabase* name_db)
    : FileList()
    , FileIndex()
    , NewMixHandle(nullptr)
    , Alignment(std::min(Power_Of_Two(alignment), 16u))
    , HasChecksum(has_checksum)
    , IsEncrypted(is_encrypted)
    , ForceFlags(force_flags)
    , Quiet(quiet)
    , Header()
    , NameDatabase(name_db)
{
    // Create a new file instance and store it.
    NewMixHandle = filename != nullptr ? new TFC(filename) : new TFC(TempFilename);
}

template <typename TFC, typename TCRC> MixFileCreatorClass<TFC, TCRC>::~MixFileCreatorClass()
{
    if (NewMixHandle) {
        delete NewMixHandle;
    }

    NewMixHandle = nullptr;
}

template <typename TFC, typename TCRC> void MixFileCreatorClass<TFC, TCRC>::Add_File(char const* filename)
{
    char path[PATH_MAX];
    path[PATH_MAX - 1] = '\0';

    TFC file(filename);

    if (!file.Is_Available() || Header.count == USHRT_MAX || (Header.size + (uint64_t)file.Size()) > ULONG_MAX) {
        DBG_INFO("File '%s' not available or you have excceeded the allowed file count or size for mix files.\n",
                 filename);
        return;
    }

    strcpy(path, filename);
    FileDataNode* filedata = new FileDataNode;

    filedata->FilePath = strdup(path);

    char* basefname = basename(path); // basename can alter string so assign after duplicated

    if (NameDatabase != nullptr) {
        NameDatabase->Add_Entry(basefname, "");
    }

    DBG_INFO("Adding '%s'.", basefname);

    filedata->Entry.CRC = Calculate_CRC<TCRC>(strupr(basefname), (unsigned)strlen(basefname));

    filedata->Entry.Offset = Header.size;
    filedata->Entry.Size = file.Size();

    // Handle calculating any padding needed to account for alignment.
    Header.size += file.Size();
    unsigned padding = To_Pad(Header.size);
    filedata->Padding = padding;
    Header.size += padding;

    DBG_INFO("File '%s' will be padded by %u bytes to fulfill alignment.", basefname, padding);

    FileList.Add_Tail(filedata);
    FileIndex.Add_Index(filedata->Entry.CRC, filedata);

    filedata->Entry.CRC = htole32(filedata->Entry.CRC);
    filedata->Entry.Offset = htole32(filedata->Entry.Offset);
    filedata->Entry.Size = htole32(filedata->Entry.Size);

    ++Header.count;
}

template <typename TFC, typename TCRC> void MixFileCreatorClass<TFC, TCRC>::Write_Mix()
{
    FileHeader tmpheader;
    uint8_t digest[20];

    PKPipe pkpipe(PKPipe::ENCRYPT, CryptRandom);
    SHAPipe shpipe;
    FilePipe flpipe(NewMixHandle);

    Pipe* pipe_to_use = &flpipe;

    int headersize = 0;
    int bodysize = 0;

    // Don't attempt to write a file if the file list is empty.
    if (FileList.Is_Empty()) {
        DBG_INFO("The file list is empty, not writing mix file.\n");
        return;
    }

    NewMixHandle->Open(WRITE);
    pkpipe.Put_To(&flpipe);

    // Write the flags and setup PKPipe if needed.
    if (IsEncrypted || HasChecksum || ForceFlags) {
        uint32_t flags = 0;

        if (IsEncrypted) {
            pkpipe.Key(&PrivateKey);
            pipe_to_use = &pkpipe;
            flags |= IS_ENCRYPTED;
        }

        if (HasChecksum) {
            flags |= HAS_CHECKSUM;
        }

        flags = htole32(flags);

        // Only use FilePipe to put the flags.
        headersize += flpipe.Put(&flags, sizeof(flags));
    }

    // Write the header, swapping byte order if needed.
    tmpheader.count = htole16(Header.count);
    tmpheader.size = htole32(Header.size);

    headersize += pipe_to_use->Put(&tmpheader, sizeof(tmpheader));

    // This forces a sort of the index which is important to get the header written in the correct order.
    FileIndex.Is_Present(0);

    // Write the index, the index should have been byte swapped as it was
    // generated if it was needed.
    for (int i = 0; i < FileIndex.Count(); ++i) {
        //DBG_INFO("Writing header entry for '%s'", NameDatabase->Get_Entry(FileIndex.Fetch_Entry(i)->Entry.CRC).file_name.c_str());
        headersize += pipe_to_use->Put(&FileIndex.Fetch_Entry(i)->Entry, sizeof(SubBlock));
    }

    // Ensure that the final block put to the file is encrypted.
    char padding[16] = {0};

    if (IsEncrypted) {
        unsigned remainder = unsigned(FileIndex.Count() * sizeof(SubBlock) + sizeof(FileHeader)) % 8;
        DBG_INFO("Padding encrypted header with %u bytes.\n", 8 - remainder);
        headersize += pipe_to_use->Put(padding, 8 - remainder);
    }

    headersize += pipe_to_use->Flush();

    // Change to either SHAPipe or plain FilePipe depending on if we need a
    // checksum.
    shpipe.Put_To(&flpipe);
    pipe_to_use = HasChecksum ? static_cast<Pipe*>(&shpipe) : static_cast<Pipe*>(&flpipe);

    // Loop through the file nodes and write the file data to the body of the
    // mix. These are written in the order they were added to the list, not the
    // sorted order that the index must be written in.
    for (FileDataNode* node = FileList.First(); node->Next() != nullptr; node = node->Next()) {
        if (!Quiet) {
            printf("Writing file %s\n", node->FilePath);
        }

        bodysize += Write_File(node->FilePath, pipe_to_use);

        if (node->Padding != 0) {
            DBG_INFO("Writing %u bytes of padding.", node->Padding);
            bodysize += pipe_to_use->Put(padding, node->Padding);
        }
    }

    // Write the checksum if needed, this will have been generated during the
    // writing of the mix body if it was required.
    if (HasChecksum) {
        shpipe.Result(&digest);
        flpipe.Put(&digest, sizeof(digest));
    }

    // If we aren't doing a quiet run, print some info about the finished mix to
    // the console.
    if (!Quiet) {
        printf(
            "Header is %d bytes, body is %d bytes, MIX contains %d files.\n", headersize, bodysize, FileIndex.Count());
    }
}

template <typename TFC, typename TCRC> int MixFileCreatorClass<TFC, TCRC>::Write_File(const char* filename, Pipe* pipe)
{
    TFC filereader(filename);
    uint8_t buffer[1024];
    int data_read;
    int total = 0;

    filereader.Open(READ);

    // We loop through the file reading chunks into our buffer and then writing
    // them to the destination pipe.
    while ((data_read = filereader.Read(buffer, 1024)) > 0) {
        total += pipe->Put(buffer, data_read);
    }

    filereader.Close();

    return total;
}

#endif // MIXCREATE_H
