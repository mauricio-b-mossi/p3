#include <fuse.h>
#include <iostream>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include "../libWad/Wad.h"

using namespace std;

int main(int argc, char* argv[]){
    if(argc < 3){
        cout << "Not enough arguments" << endl;
        return 0;
    }

    string wadPath = argv[argc - 2];

    if(wadPath.at(0) != '/'){
        wadPath = string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;

    //((Wad*)fuse_get_context()->private_data)
    return fuse_main(argc, argv, &operations, myWad);

}