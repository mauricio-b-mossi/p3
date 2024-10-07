#pragma once
#include <string>
#include <vector>
#include <regex>

using namespace std;



class FsObj{

    string name;
    vector<FsObj*> children;
    int offset;
    int length;
    int pos;
    int _end = -1;

    public:
        FsObj();
        FsObj(const string &name, int offset, int length, int pos);
        void setEnd(int _end); // Points to the end of namespace.
        void appendChild(FsObj* child);
        bool isMapDirectory();
        bool isNamespaceDirectory();
        int getNumChildren();
        int getLength();
        int getPosition();
        int getEnd();
        int getOffset();
        string getName();
        //string getContent();
        vector<string> getChildrenNames();
        vector<FsObj*> getChildren();
        void clear();
        void traverse(bool root = false, string prev = "");
};

class Wad {

    string magic;
    string path;
    unsigned int descriptorListOffset;
    unsigned int descriptorListLength;
    FsObj* root = nullptr;
    FsObj* getPathItem(const string &path);
    bool isDirectoryFromName(const string &name);
    vector<string> parsePath(const string &path);
    string normalizePath(const string &path);
    bool isAbsolutePathAndNotEmpty(const string &path);
    int getInsertionPosition(const string &path);

    public:
        Wad(const string &path);
        ~Wad();

        static Wad* loadWad(const string &path);
        static regex mapPattern;
        static regex namespaceStartPattern;
        static regex namespaceEndPattern;

        void reloadWad();

        string getMagic();
        bool isContent(const string &path);
        bool isDirectory(const string &path);
        int getSize(const string &path);
        int getContents(const string &path, char *buffer, int length, int offset = 0);
        int getDirectory(const string &path, vector<string> *directory);

        void createDirectory(const string &path);
        void createFile(const string &path);
        int writeToFile(const string &path, const char *buffer, int length, int offset = 0);
};

struct FileIO{
    static void shift(const string& filename, streamoff offset, streamoff shiftAmount);

    static void append(const string &filename, const string &data);

    static void write(const string &filename, const string &data);

    static void writeAtLocation(const string& filename, streamoff offset, const string& data);
};

struct FileDescriptor {
    unsigned int elementOffset;
    unsigned int elementLength;
    char nameBuffer[8]; // To hold the name exactly 8 bytes

    bool createFileDescriptor(unsigned int elementOffset, unsigned int elementLength, const string &name);

    // Method to return the complete 16-byte string representation
    std::string toString() const;
};