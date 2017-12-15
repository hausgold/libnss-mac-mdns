#!/bin/bash
#
# This script will perform the installation of the libnss-mac-mdns NSS
# extension. It has the same requirements as described on the readme file.
# We build the extension, install it to the system and edit your
#
# This script was tested on Ubuntu Trusty (14), but has no OS/Distro specific
# behavior on tooling. So it should work on most Linux distributions.
#
# This script should be called like this:
#
#   $ curl 'https://raw.githubusercontent.com/hausgold/libnss-mac-mdns/master/install.sh' | bash
#
# Used Ubuntu packages: wget libglib2.0-dev build-essential
#
# @author Hermann Mayer <hermann.mayer92@gmail.com>

# Fail on any errors
set -eE

# Delete all temporary stuff
cd /tmp
rm -rf libnss-mac-mdns*

# Download the latest libnss-mac-mdns version
wget \
  -O libnss-mac-mdns.tar.gz \
  https://github.com/hausgold/libnss-mac-mdns/archive/master.tar.gz

# Extract the source code package
mkdir libnss-mac-mdns
tar xfv libnss-mac-mdns.tar.gz --strip-components=1 -C libnss-mac-mdns
cd libnss-mac-mdns

# Build the package
make build

# Install the package
make install

# Reconfigure the nsswitch.conf
sed -i -re 's/^hosts:(.*) dns(.*)/hosts:\1 mac_mdns dns\2/' /etc/nsswitch.conf

# Go back to the temporary directory
cd /tmp

# Delete all temporary stuff
rm -rf libnss-mac-mdns*
