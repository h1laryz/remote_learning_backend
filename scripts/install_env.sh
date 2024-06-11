sudo apt-get update -y
sudo apt-get upgrade -y

sudo apt-get install -y git make cmake gcc g++

#cd && \
#git clone --recurse-submodules https://github.com/userver-framework/userver && \
#cd userver && \
#sudo apt install -y $(cat ./scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ') && \
#cmake -S./ -B./build_release \
#    -DCMAKE_BUILD_TYPE=Release \
#    -DUSERVER_INSTALL=ON \
#    -DUSERVER_FEATURE_POSTGRESQL=ON \
#    -GNinja && \
#cmake --build build_release/ && \
#cmake --install build_release/ && \
#sudo ldconfig && \
#cd && \
#rm -rf userver

cd && \
git clone https://github.com/Thalhammer/jwt-cpp && \
cd jwt-cpp && \
mkdir build && \
cd build && \
cmake .. && \
sudo make install && \
cd && \
rm -rf jwt-cpp && \
sudo ldconfig && \
cd && \
rm -rf jwt-cpp


cd && \
git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
cd aws-sdk-cpp && \
mkdir build && \
cd build && \
cmake .. \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_ONLY="s3" && \
sudo make install && \
sudo ldconfig && \
cd && \
rm -rf aws-sdk-cpp

cd && \
git clone --recurse-submodules https://github.com/Neargye/magic_enum && \
cd magic_enum && \
mkdir build && \
cd build && \
cmake .. \
-DCMAKE_BUILD_TYPE=Release && \
sudo make install && \
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/aws-sdk.conf && \
sudo ldconfig && \
cd && \
rm -rf magic_enum
