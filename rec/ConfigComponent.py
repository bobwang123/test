#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author  : zhuzhzh
# @Time    : 2017/12/9 0009 16:02:23
# @File    : ConfigComponent.py
# @Description:


from configparser import ConfigParser
import os


class CommConfig:
    cp = ConfigParser()

    curr_dir = os.path.dirname(os.path.realpath(__file__)) + os.sep
    cp.read(curr_dir + "config.ini")

    def getConfig(self):
        return self.cp;
