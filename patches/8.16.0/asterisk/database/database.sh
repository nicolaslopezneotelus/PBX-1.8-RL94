#!/bin/bash
createdb asterisk
psql asterisk -f script.txt

