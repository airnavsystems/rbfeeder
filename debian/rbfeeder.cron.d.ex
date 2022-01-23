#
# Regular cron jobs for the rbfeeder package
#
0 4	* * *	root	[ -x /usr/bin/rbfeeder_maintenance ] && /usr/bin/rbfeeder_maintenance
