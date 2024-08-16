#!/bin/bash
# VACUUM reclaims storage occupied by deleted tuples. In normal PostgreSQL operation, tuples that are deleted or obsoleted by an update
# are not physically removed from their table; they remain present until a VACUUM is done. Therefore it's necessary to do VACUUM periodically,
# especially on frequently-updated tables.
# FULL: Selects "full" vacuum, which may reclaim more space, but takes much longer and exclusively locks the table.
# ANALYZE: Updates statistics used by the planner to determine the most efficient way to execute a query.

vacuumdb -a -f -z -U postgres