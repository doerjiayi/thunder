#!/bin/bash
find ./ -maxdepth 3 -type f -name "*.json"  |xargs sed -i "s/192.168.2.129/192.168.2.129/g"
find ./ -maxdepth 3 -type f -name "*.conf"  |xargs sed -i "s/192.168.2.129/192.168.2.129/g"
