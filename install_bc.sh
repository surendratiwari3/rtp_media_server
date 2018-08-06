# https://github.com/jchavanton/rtp_media_server.git
apt-get install automake

# bcunit
git clone https://github.com/BelledonneCommunications/bcunit.git
git checkout 29c556fa8ac1ab21fba1291231ffa8dea43cf32a
./autogen.sh
./configure
make
make install
cd ..

# bctoolbox
apt-get install libmbedtls-dev
git clone https://github.com/BelledonneCommunications/bctoolbox.git
git checkout 971953a9fa4058e9c8a40ca4a3fa12d832445255
./autogen.sh
./configure
make
make install
cd ..

# oRTP
git clone https://github.com/BelledonneCommunications/ortp.git
git checkout 6e13ef49a55cdd19dae395c38cfff7ffa518a089
cd ortp
./autogen.sh
./configure
make
make install

# mediastreamer2
apt-get install intltool libspeex-dev libspeexdsp-dev
git clone https://github.com/BelledonneCommunications/mediastreamer2.git
git checkout d935123fc497d19a24019c6e7ae4fe0c5f19d55a
./autogen.sh
./configure --disable-video --enable-tools=no --disable-tests
make
make install
