#pragma once
#include <string>
#include <vector>
#include <regex>

using namespace std;



class FsObj{

    string name;
    vector<FsObj*> children;
    string content;
    int offset;
    int length;
    int _end = -1;

    public:
        FsObj();
        FsObj(string name, string content);
        void appendChild(FsObj* child);
        bool isMapDirectory();
        bool isNamespaceDirectory();
        int getNumChildren();
        string getName();
        string getContent();
        vector<string> getChildrenNames();
        vector<FsObj*> getChildren();
};

class Wad {

    string path; 
    string magic;
    unsigned int descriptorListOffset;
    unsigned int descriptorListLength;
    FsObj* root;
    FsObj* getPathItem(const string &path);
    bool isDirectoryFromName(const string &name);
    vector<string> parsePath(const string &path);
    string normalizePath(const string &path);
    bool isAbsolutePathAndNotEmpty(const string &path);
    int getInsertionPosition(const string &path);

    public:
        Wad(const string &path);

        static Wad* loadWad(const string &path);
        static regex mapPattern;
        static regex namespaceStartPattern;
        static regex namespaceEndPattern;

        string getMagic();
        bool isContent(const string &path);
        bool isDirectory(const string &path);
        int getSize(const string &path);
        int getContents(const string &path, char *buffer, int length, int offset);
        int getDirectory(const string &path, vector<string> *directory);

        void createDirectory(const string &path);
        void createFile(const string &path);
        int writeToFile(const string &path, const char *buffer, int length, int offset);
};