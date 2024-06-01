sudo apt-get update -y
sudo apt-get upgrade -y

sudo apt-get install -y git make cmake gcc g++

cd && \
git clone https://github.com/Thalhammer/jwt-cpp && \
cd jwt-cpp && \
mkdir build && \
cd build && \
cmake .. && \
sudo make install && \
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
sudo make install

cd && \
git clone --recurse-submodules https://github.com/Neargye/magic_enum && \
cd magic_enum && \
mkdir build && \
cd build && \
cmake .. \
-DCMAKE_BUILD_TYPE=Release && \
sudo make install

echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/aws-sdk.conf
sudo ldconfig
