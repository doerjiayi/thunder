#!/bin/bash 
echo "cpu:"
echo "lscpu"
lscpu
echo "cat /proc/cpuinfo"
cat /proc/cpuinfo

echo "memory:"
echo "free -m"
free -m
echo "cat /proc/meminfo"
cat /proc/meminfo

echo "disk:"
echo "lsblk"
lsblk

echo "network:"
echo "查看网卡硬件信息 lspci | grep -i 'eth'"
lspci | grep -i 'eth'
echo "查看系统的所有网络接口 ifconfig -a"
ifconfig -a
echo "ip link show"
ip link show
echo "sudo ethtool eth0"
sudo ethtool eth0