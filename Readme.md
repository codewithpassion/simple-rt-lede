
    make package/vpn-reverse-tether/compile V=s



add kmod-tun
(create /dev/net/ dir && mknod /dev/net/tun c 10 200 // solved probably by kmod-tun)


make package/SimpleRT2/compile V=s && scp /media/roboto/_seagate/lede/lede-sdk-ar71xx-nand_gcc-5.4.0_musl.Linux-x86_64/bin/packages/mips_24kc/base/simple-rt-cli_0.2-2_mips_24kc.ipk root@10.1.1.1:/tmp/
