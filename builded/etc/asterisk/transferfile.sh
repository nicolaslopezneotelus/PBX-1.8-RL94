#!/usr/local/bin/kermit +
#/etc/asterisk/transferfile.sh /var/spool/asterisk/monitor/1000/MON-455900-2d27005485204853ae4217dcf14e4c2a-in.wav 1000

.FTPHOST := "10.88.8.10"
.FTPPORT := "26"
.FTPUSER := "FTPUSER"
.FTPPASSWORD := "-"
.FTPREMOTEDIR := ""
.DEBUGLEVEL = 1
.LOGLEVEL = 1
.LOGFILE := "transferfile.log"
.COMMAND := \%0 \%*

if < \v(argc) 2 exit 1 Usage: \%0 outfile remotesubdir

#Output file, setted by neotel.
.outfile := \fcontents(\%1)
#Remote subdirectory alias, setted by neotel
.remotesubdir := \%2
#FTP remote directory alias.
#E.g.: /SISTEMA/1000/
.remotedir := \m(FTPREMOTEDIR)\m(remotesubdir)/

#for \%i 1 \v(argc)-1 1 {
#  if = \%i 1 {
#    .infile1 := \fcontents(\&_[\%i])
#  }
#}

define openlog {
  if \m(LOGLEVEL) open append \m(LOGFILE)
}
define logline {
  if \m(LOGLEVEL) {
    writeln file {\v(date) \v(time) \m(COMMAND)}
    writeln file {\v(date) \v(time) \%1}
  }
}
define debugline {
  if \m(DEBUGLEVEL) echo {\%1}
}

#Open log file.
openlog

.outfilename := \fbasename(\m(outfile))

#Check if outfile exists.
if not exist \m(outfile) {
  debugline {\m(outfile): File not found}
  logline {\m(outfile): File not found}
  exit 1
}

#Check if outfile is readable.
if not readable \m(outfile) {
  debugline {\m(outfile): File not readable}
  logline {\m(outfile): File not readable}
  exit 1
}


#Open FTP connection.
debugline {ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active}
ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active

#Check connection state.
if fail {
  debugline {ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active failed}
  logline {ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active failed}
  exit 1
}

#Check successful login.
if not \v(ftp_loggedin) {
  debugline {login failed}
  logline {login failed}
  exit 1
}

#Change directory on FTP to remotedir.
debugline {ftp cd \m(remotedir)}
ftp cd \m(remotedir)

#Check FTP command status.
if fail {
  debugline {ftp cd \m(remotedir) failed}
  logline {ftp cd \m(remotedir) failed}
  ftp bye
  exit 1
}

#Upload outfile.
debugline {ftp put \m(outfile)}
ftp put \m(outfile)

#Check FTP command status.
if fail {
  debugline {ftp put \m(outfile) failed}
  logline {ftp put \m(outfile) failed}
  ftp bye
  exit 1
}

#Logout and disconnect.
debugline {ftp bye}
ftp bye

if exist \m(outfile) rm \m(outfile)

#Exit successfully (status code 0) from the script.
exit 0