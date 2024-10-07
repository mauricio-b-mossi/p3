#include "Wad.hpp"
#include <stack>
#include <iostream>
#include <fstream>
#include <cctype>
#include <regex>
#include <sstream>
#include <stdexcept>

using namespace std;

regex Wad::namespaceStartPattern("_START$");
regex Wad::namespaceEndPattern("_END$");
regex Wad::mapPattern("^E[0-9]M[0-9]$");

// Even though advised against in video, I've decided to load fileContent into memory for simplicity.
Wad::Wad(const string &path) : path{path}{
    ifstream file(path, ios::binary);

    /*
    - Header (FIXED 12) 4: magic, 4: descriptorsListLength, 4: descriptorsListOffset.
    - Lump Data (DYNAMIC)
    - Descriptors List (DYNAMIC)
        - File Descriptor (FIXED 16) 4: elementOffset, 4: elementLength, 8: name. 
    */ 

    if (!file.is_open()){
     throw runtime_error("Unable to open file");
    }

    char* magic = new char[4];
    unsigned int descriptorsListLength;
    unsigned int descriptorListOffset;

    file.read(magic, 4);
    file.read((char*)&descriptorsListLength, 4);
    file.read((char*)&descriptorListOffset, 4);

    this->magic = string(magic, 4);
    this->descriptorListOffset = descriptorListOffset;
    this->descriptorListLength = descriptorsListLength;

    stack<FsObj*> directoryStucture;
    FsObj* root = new FsObj();
    directoryStucture.push(root);

    for (int i = 0; i < descriptorsListLength; i++) {

        file.seekg(descriptorListOffset + i * 16, ios_base::beg); // Going to the ith item in the descriptor list.
        unsigned int elementOffset;
        unsigned int elementLength;
        char* nameBuffer = new char[8];

        file.read((char*)&elementOffset, 4);
        file.read((char*)&elementLength, 4);
        file.read(nameBuffer, 8);
        string elementName(nameBuffer, 8);
        delete[] nameBuffer;

        if (directoryStucture.top()->isMapDirectory() && directoryStucture.top()->getNumChildren() == 10) { 
            directoryStucture.pop();
        }

        else if (regex_search(elementName, Wad::namespaceStartPattern)) // Starting namespace case.
        {
            auto markerStart = elementName.find("_START"); // I know this wont fail by regex.
            auto namespaceDirectoryName = elementName.substr(0, markerStart);
            FsObj* namespaceDirectory = new FsObj(namespaceDirectoryName, "");

            directoryStucture.top()->appendChild(namespaceDirectory);
            directoryStucture.push(namespaceDirectory);

        }

        else if (regex_search(elementName, Wad::namespaceEndPattern)) // Closing namespace case.
        {
            directoryStucture.pop();
        }

        else if(regex_match(elementName, Wad::mapPattern)) // Matching regex ensures name is proper map directory.
        {
            FsObj* mapDirectory = new FsObj(elementName, "");
            directoryStucture.top()->appendChild(mapDirectory);
            directoryStucture.push(mapDirectory);
        }

        else // If not any of the above, then it is file, might be file of length 0.
        {
            file.seekg(elementOffset, ios_base::beg);
            char* contentBuffer = new char[elementLength];
            file.read(contentBuffer, elementLength);

            stringstream ss;

            for (int i = 0; i < elementLength; i++) {
                ss << contentBuffer[i];
            }

            delete[] contentBuffer;

            auto fileContent = ss.str();

            FsObj* newFile = new FsObj(elementName, fileContent);

            directoryStucture.top()->appendChild(newFile);
        }
    }

    this->root = root;

    file.close();
} 

vector<string> Wad::parsePath(const string& path) {
    vector<string> parts;
    stringstream ss(path);
    string part;

    if (!path.empty() && path[0] == '/') { // Adding root.
        parts.push_back("/");
    }

    while (getline(ss, part, '/')) { // Split the rest normally, removing trailing /.
        if (!part.empty()) { 
            parts.push_back(part);
        }
    }

    return parts;
}

string Wad::normalizePath(const string &path) // Normalizes path by removing trailing slashes.
{ 
    if (path == "/" || path.empty()) {
        return path;
    }

    size_t end = path.find_last_not_of('/');
    return path.substr(0, end + 1);
}

bool Wad::isAbsolutePathAndNotEmpty(const string &path)
{
    return path.size() > 0 && path.at(0) == '/';
}

// Could cause errors.
Wad* Wad::loadWad(const string &path) {
    return new Wad(path);
}


string Wad::getMagic() {
    return this->magic;
}

bool Wad::isDirectoryFromName(const string &name){
    return regex_match(name, Wad::mapPattern) || regex_search(name, Wad::namespaceStartPattern) || regex_search(name, Wad::namespaceEndPattern);
}

bool Wad::isDirectory(const string &path) {
    if(!isAbsolutePathAndNotEmpty(path)){
        return false;
    }
    FsObj* pathItem = getPathItem(normalizePath(path));

    if(pathItem == nullptr){
        return false;
    }

    return this->isDirectoryFromName(pathItem->getName());
}

bool Wad::isContent(const string &path) {
    if(!isAbsolutePathAndNotEmpty(path)){
        return false;
    }

    FsObj* pathItem = getPathItem(normalizePath(path));

    if(pathItem == nullptr){
        return false;
    }

    return !this->isDirectoryFromName(pathItem->getName());
}


int Wad::getSize(const string &path) {
    if(!isAbsolutePathAndNotEmpty(path)){
        return -1;
    }

    FsObj* pathItem = getPathItem(normalizePath(path));

    if (pathItem == nullptr){
        return -1;
    }

    if(this->isDirectoryFromName(pathItem->getName())){
        return -1;
    }

    return pathItem->getContent().length(); // If file of 0 length?
}


int Wad::getContents(const string &path, char *buffer, int length, int offset = 0) {
    if(!isAbsolutePathAndNotEmpty(path)){
        return -1;
    }

    FsObj* pathItem = getPathItem(normalizePath(path));

    if (pathItem == nullptr){
        return -1;
    }

    if(this->isDirectoryFromName(pathItem->getName())){
        return -1;
    }

    if(offset >= pathItem->getContent().length()){
        return 0;
    }

    if (length > pathItem->getContent().length()) {
        length = pathItem->getContent().length();
    }

    string content(pathItem->getContent(), offset, length);

    for (int i = 0; i < content.size(); i++) {
        buffer[i] = content.at(i);
    }

    return content.size(); // Double check.
}


int Wad::getDirectory(const string &path, vector<string> *directory) {
    if(!isAbsolutePathAndNotEmpty(path)){
        return -1;
    }

    FsObj* pathItem = getPathItem(normalizePath(path));

    if (pathItem == nullptr){
       return -1; 
    } 

    if(!this->isDirectoryFromName(pathItem->getName())){
        return -1;
    }

    for (string childName : pathItem->getChildrenNames()) {
        directory->push_back(childName);
    }
    
    return pathItem->getNumChildren();
}

void Wad::createDirectory(const string &path){
    if(!isAbsolutePathAndNotEmpty(path)){
        return;
    }

    FsObj* pathItem = getPathItem(normalizePath(path));
    
    if(pathItem != nullptr){ // Directory already exists
        return;
    }

    // Directory does not exist. Check if length is 2
    vector<string> parsedPath = parsePath(path);

    if(parsedPath.size() < 2){ // just contains "/"
        return;
    }

    if(parsedPath.back().length() > 2){ // name does not fit 2 char constraint.
        return;
    }

    // check if parent valid, exists and not map or file.
    size_t pos = path.rfind(parsedPath.back());

    string parent;

    if (pos != std::string::npos) {
        parent = normalizePath(path.substr(0, pos));
    }

    if(parent.empty()){
        return;
    }

    pathItem = getPathItem(parent);

    if(pathItem )

    // valid insertion. Search position insert 32 bytes.
}

void Wad::createFile(const string &path){
    return;
}

int Wad::writeToFile(const string &path, const char *buffer, int length, int offset = 0){
    return 0;
}

FsObj* Wad::getPathItem(const string &path) {
    vector<string> pathItemNames;
    stringstream ss(path);
    string nameBuffer;

    if (path.length() == 1 && path.at(0) == '/') {
        return this->root;
    }

    while (getline(ss, nameBuffer, '/')) { // Break into pathItemNames
        pathItemNames.push_back(nameBuffer);
    }


    // going to use curr as pointer to traverse structure. Will return curr as a pointer
    // to final destination.
    FsObj* curr = this->root; 

    for (int i = 1; i < pathItemNames.size(); i++) { // ignoring first item since "/" -> [""].

        vector<FsObj*> children = curr->getChildren();
        bool pathItemFound = false;

        for (auto child : children) {
            if (child->getName() == pathItemNames.at(i)) {
                curr = child; 
                pathItemFound = true;
                break;
            }
        }

        if (!pathItemFound) {
            return nullptr;
        }
    }

    return curr;
}

FsObj::FsObj() : name{""}, content{""} {}


FsObj::FsObj(string name, string content) : name{name}, content{content} {}


void FsObj::appendChild(FsObj* child) {
    this->children.push_back(child);
}

// Regex: match, matches the whole char seq, search, matches any part of the char seq.

bool FsObj::isMapDirectory() {
    return regex_match(this->name, Wad::mapPattern);
}

bool  FsObj::isNamespaceDirectory(){
    return regex_search(this->name, Wad::namespaceStartPattern);
}


int FsObj::getNumChildren() {
    return this->children.size();
}


string FsObj::getName() {
    return this->name;
}


string FsObj::getContent() {
    return this->content;
}

vector<string> FsObj::getChildrenNames() {
    vector<string> childrenNames;

    for (auto child : this->getChildren()) {
        childrenNames.push_back(child->getName());
    }

    return childrenNames;
}

vector<FsObj*> FsObj::getChildren() {
    return this->children;
}