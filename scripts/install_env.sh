apt-get update -y 
apt-get upgrade -y

apt-get install -y sudo git make cmake gcc g++

cd && \
git clone https://github.com/Thalhammer/jwt-cpp && \
cd jwt-cpp && \
mkdir build && \
cd build && \
cmake .. -G Ninja && \
sudo make install && \
cd && \
rm -rf jwt-cpp 

cd && \
git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
cd aws-sdk-cpp && \
mkdir build && \
cd build && \
cmake .. -G Ninja && \
-DCMAKE_BUILD_TYPE=Release && \
-DBUILD_ONLY="s3" \
sudo make install
