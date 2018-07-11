#!/bin/bash

sudo route del -net 0.0.0.0 gw 192.168.0.1 dev eth0
python overlay_server.py
