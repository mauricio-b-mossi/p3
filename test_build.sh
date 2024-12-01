echo "\n======== Untarring and unzipping Library and Daemon. ========\n"

tar zxvf wad.tar.gz
cd libWad
make
cd ..
cd wadfs
make
cd ..

echo "\n======== Running tests. ========\n"

if [ ! -d "./test-workspace" ]; then
    echo "\n======== Untarring and unzipping Test Suite. ========\n"
    tar -zxvf ./P3_LibraryTestSuite.tgz
fi  # Close the if block properly

sudo chmod +x ./run_libtest.sh
sudo ./run_libtest.sh