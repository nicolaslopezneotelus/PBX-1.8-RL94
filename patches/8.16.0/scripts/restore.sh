#!/bin/bash

tar -zxvPf backup.tar.gz
#tar -xvPf backup.tar
psql asterisk < asterisk.bk --username postgres

