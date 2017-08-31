
    make package/vpn-reverse-tether/compile V=s



add kmod-tun
(create /dev/net/ dir && mknod /dev/net/tun c 10 200 // solved probably by kmod-tun)
