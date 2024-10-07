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
    FsObj* root = new FsObj("/", 0, 0, 0);
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

        if (regex_search(elementName, Wad::namespaceStartPattern)) // Starting namespace case.
        {
            auto markerStart = elementName.find("_START"); // I know this wont fail by regex.
            auto namespaceDirectoryName = elementName.substr(0, markerStart);
            FsObj* namespaceDirectory = new FsObj(namespaceDirectoryName, elementOffset, elementLength, i);

            directoryStucture.top()->appendChild(namespaceDirectory);
            directoryStucture.push(namespaceDirectory);

        }

        else if (regex_search(elementName, Wad::namespaceEndPattern)) // Closing namespace case.
        {
            directoryStucture.top()->setEnd(i);
            directoryStucture.pop();
        }

        else if(regex_match(elementName, Wad::mapPattern)) // Matching regex ensures name is proper map directory.
        {
            FsObj* mapDirectory = new FsObj(elementName, elementOffset, elementLength, i);
            directoryStucture.top()->appendChild(mapDirectory);
            directoryStucture.push(mapDirectory);
        }

        else // If not any of the above, then it is file, might be file of length 0.
        {
            FsObj* newFile = new FsObj(elementName, elementOffset, elementLength, i);

            directoryStucture.top()->appendChild(newFile);
        }
    }

    this->root = root;

    file.close();
} 

// Could cause errors.
Wad* Wad::loadWad(const string &path) {
    return new Wad(path);
}

void Wad::reloadWad(){
    if(this->root == nullptr){
        return;
    }

    this->root->clear();

    ifstream file(this->path, ios::binary);

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
    directoryStucture.push(this->root);

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

        if (regex_search(elementName, Wad::namespaceStartPattern)) // Starting namespace case.
        {
            auto markerStart = elementName.find("_START"); // I know this wont fail by regex.
            auto namespaceDirectoryName = elementName.substr(0, markerStart);
            FsObj* namespaceDirectory = new FsObj(namespaceDirectoryName, elementOffset, elementLength, i);

            directoryStucture.top()->appendChild(namespaceDirectory);
            directoryStucture.push(namespaceDirectory);

        }

        else if (regex_search(elementName, Wad::namespaceEndPattern)) // Closing namespace case.
        {
            directoryStucture.top()->setEnd(i);
            directoryStucture.pop();
        }

        else if(regex_match(elementName, Wad::mapPattern)) // Matching regex ensures name is proper map directory.
        {
            FsObj* mapDirectory = new FsObj(elementName, elementOffset, elementLength, i);
            directoryStucture.top()->appendChild(mapDirectory);
            directoryStucture.push(mapDirectory);
        }

        else // If not any of the above, then it is file, might be file of length 0.
        {
            FsObj* newFile = new FsObj(elementName, elementOffset, elementLength, i);

            directoryStucture.top()->appendChild(newFile);
        }
    }

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

    return pathItem->getSize(); // If file of 0 length?
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

    if(offset >= pathItem->getSize()){
        return 0;
    }

    if (length > pathItem->getSize()) {
        length = pathItem->getSize();
    }

    ifstream file(this->path, ios::binary);
    file.seekg(pathItem->getOffset() + offset, ios_base::beg);

    auto effectiveLength = pathItem->getSize() - offset;

    char* contentBuffer = new char[effectiveLength];
    file.read(contentBuffer, effectiveLength);

    stringstream ss;

    for (int i = 0; i < effectiveLength; i++) {
        ss << contentBuffer[i];
    }

    delete[] contentBuffer;

    string content =  ss.str();

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

    string directoryName = parsedPath.back();

    if(directoryName.length() > 2){ // name does not fit 2 char constraint.
        return;
    }

    // check if parent valid, exists and not map or file.
    size_t pos = path.rfind(directoryName);

    string parent;

    if (pos != std::string::npos) {
        parent = normalizePath(path.substr(0, pos));
    }

    if(parent.empty()){
        return;
    }

    pathItem = getPathItem(parent);

    if(pathItem == nullptr){
        return;
    }

    // Insert either on "/" or namespace.
    if(pathItem->getName() == "/")
    {
        this->descriptorListLength += 2;

        ostringstream oss;
        
        oss.write(reinterpret_cast<const char*>(&this->descriptorListLength), sizeof(this->descriptorListLength));

        string result = oss.str();

        FileIO::writeAtLocation(this->path, 4, result);

        FileDescriptor startFd;
        FileDescriptor endFd;

        startFd.createFileDescriptor(0, 0, directoryName + "_START");
        endFd.createFileDescriptor(0, 0, directoryName + "_END");

        FileIO::append(this->path, startFd.toString());
        FileIO::append(this->path, endFd.toString());
    }
    else if(pathItem->isNamespaceDirectory())
    {
        this->descriptorListLength += 2;

        ostringstream oss;
        
        oss.write(reinterpret_cast<const char*>(&this->descriptorListLength), sizeof(this->descriptorListLength));

        string result = oss.str();

        FileIO::writeAtLocation(this->path, 4, result);

        FileDescriptor startFd;
        FileDescriptor endFd;

        startFd.createFileDescriptor(0, 0, directoryName + "_START");
        endFd.createFileDescriptor(0, 0, directoryName + "_END");

        auto effectiveOffset = pathItem->getEnd() * 16 + this->descriptorListOffset;

        FileIO::shift(this->path, effectiveOffset, 32);
        FileIO::writeAtLocation(this->path, effectiveOffset, startFd.toString());
        FileIO::writeAtLocation(this->path, effectiveOffset + 16, endFd.toString());
    }
}

void Wad::createFile(const string &path){
    if(path.back() == '/'){
        return;
    }

    if(!isAbsolutePathAndNotEmpty(path)){
        return;
    }

    FsObj* pathItem = getPathItem(path);
    
    if(pathItem != nullptr){ // File already exists
        return;
    }

    // File does not exist. Check if length of path >= 2 (/<file>)
    vector<string> parsedPath = parsePath(path);

    if(parsedPath.size() < 2){ // just contains "/"
        return;
    }

    string fileName = parsedPath.back();

    if(fileName.length() > 8){ // name does not fit 8 char constraint.
        return;
    }

    // check if parent valid, exists and not map or file.
    size_t pos = path.rfind(fileName);

    string parent;

    if (pos != std::string::npos) {
        parent = normalizePath(path.substr(0, pos));
    }

    if(parent.empty()){
        return;
    }

    pathItem = getPathItem(parent);

    if(pathItem == nullptr){
        return;
    }

    // Insert either on "/" or namespace.
    if(pathItem->getName() == "/")
    {
        this->descriptorListLength += 1;

        ostringstream oss;
        
        oss.write(reinterpret_cast<const char*>(&this->descriptorListLength), sizeof(this->descriptorListLength));

        string result = oss.str();

        FileIO::writeAtLocation(this->path, 4, result);

        FileDescriptor fileFd;

        fileFd.createFileDescriptor(0, 0, fileName);

        FileIO::append(this->path, fileFd.toString());
    }
    else if(pathItem->isNamespaceDirectory())
    {
        this->descriptorListLength += 1;

        ostringstream oss;
        
        oss.write(reinterpret_cast<const char*>(&this->descriptorListLength), sizeof(this->descriptorListLength));

        string result = oss.str();

        FileIO::writeAtLocation(this->path, 4, result);

        FileDescriptor fileFd;

        fileFd.createFileDescriptor(0, 0, fileName);

        auto effectiveOffset = pathItem->getEnd() * 16 + this->descriptorListOffset;

        FileIO::shift(this->path, effectiveOffset, 16);
        FileIO::writeAtLocation(this->path, effectiveOffset, fileFd.toString());
    }
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

FsObj::FsObj() : name{""}, offset{0}, length{0}, pos{0}, _end{-1} {}


FsObj::FsObj(const string &name, int offset, int length, int pos) : name{name}, offset{offset}, length{length}, pos{pos}, _end{-1} {}

void FsObj::clear(){
    if(this->children.empty()){
        return;
    }
    for(auto child : this->children){
        child->clear();
        delete child;
    }
}

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

int FsObj::getEnd(){
    return this->_end;
}

int FsObj::getOffset(){
    return this->offset;
}

void FsObj::setEnd(int _end){
    this->_end = _end;
}

int FsObj::getSize(){
    return this->length;
}

int FsObj::getPosition(){
    return this->pos;
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

void FileIO::shift(const string& filename, streamoff offset, streamoff shiftAmount) {
    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open()) {
        throw runtime_error("Unable to open file");
    }

    // Move to the specified offset
    file.seekg(offset, ios::beg);

    // Read the rest of the file into a buffer
    vector<char> buffer;
    file.seekg(0, ios::end);
    streamoff fileSize = file.tellg();
    if (offset >= fileSize) {
        cout << "Offset is beyond the end of the file." << endl; // Append to the end.
        return;
    }

    streamoff readSize = fileSize - offset;
    buffer.resize(readSize);

    file.seekg(offset, ios::beg);
    file.read(buffer.data(), readSize);

    // Ensure the write location is correct
    file.seekp(offset + shiftAmount, ios::beg);

    // Write the buffer back to the file shifted by the amount
    file.write(buffer.data(), buffer.size());

    // Adjust the file size to account for the shift
    file.close();
    fstream truncateFile(filename, ios::in | ios::out | ios::binary);
    truncateFile.seekp(offset + shiftAmount + buffer.size(), ios::beg);
    truncateFile.close();
}

void FileIO::append(const string &filename, const string &data){
    ofstream file(filename, ios::app);

    if (!file.is_open()) {
        throw runtime_error("Unable to open file");
    }

    file << data;

    file.close();
    
    cout << "Data added" << endl;
}

void FileIO::write(const string &filename, const string &data){
    ofstream file(filename);

    if (!file.is_open()) {
        throw runtime_error("Unable to open file");
    }


    file << data;

    file.close();
    
    cout << "Data added" << endl;
}

void FileIO::writeAtLocation(const string& filename, streamoff offset, const string& data) {

    fstream file(filename, ios::in | ios::out | ios::binary);

    if (!file.is_open()) {
        throw runtime_error("Unable to open file");
    }
    

    file.seekp(offset, ios::beg);
    
    if (!file.good()) {
        file.close();
        throw runtime_error("Failed to seek to the specified position.");
    }

    file.write(data.c_str(), data.size());

    if (!file.good()) {
        throw runtime_error("Failed to write data to the file.");
    }

    file.close();
}

string FileDescriptor::toString() const {
    ostringstream oss;
    
    // Writing the elementOffset (4 bytes) and elementLength (4 bytes)
    oss.write(reinterpret_cast<const char*>(&elementOffset), sizeof(elementOffset));
    oss.write(reinterpret_cast<const char*>(&elementLength), sizeof(elementLength));
    
    // Writing the nameBuffer (8 bytes), ensuring exactly 8 characters
    oss.write(nameBuffer, sizeof(nameBuffer));
    
    // Ensure that the string is exactly 16 bytes
    string result = oss.str();
    if (result.size() != 16) {
        result.resize(16, '\0');  // Pad with null characters if necessary
    }
    
    return result;
}

bool FileDescriptor::createFileDescriptor(unsigned int elementOffset, unsigned int elementLength, const string &name){
    if(name.size() > 8){
        return false;
    }
    this->elementOffset = elementOffset;
    this->elementLength = elementLength;
    std::strncpy(this->nameBuffer, name.c_str(), name.size());
    return true;
}