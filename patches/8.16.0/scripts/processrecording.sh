#!/usr/local/bin/kermit +
#/etc/asterisk/processrecording.sh /var/spool/asterisk/recording/1000/REC-455900-20111026132600.wav PABLO-455900-15.mp3 1 1000

.MONITOR_PATH := "/var/spool/asterisk/monitor/"
.FTPHOST := "10.88.8.10"
.FTPPORT := "26"
.FTPUSER := "FTPUSER"
.FTPPASSWORD := "-"
.FTPREMOTEDIR := ""
#LOGLEVEL=[0: OFF]|[1: ERROR]|[2: INFORMATION]|[3: DEBUG]
.LOGLEVEL := 1
.LOGDIR := "/test/"
.LOGFILE := "processrecording.log"
.COMMAND := \%0 \%*

if < \v(argc) 4 exit 1 Usage: \%0 infile outfilename quality remotesubdir

#Input file, setted by neotel.
.infile := \fcontents(\%1)
#Output file, setted by neotel.
.outfilename := \fcontents(\%2)
#Quality for mp3 format, setted by neotel.
.quality := \%3
#Remote subdirectory alias, setted by neotel.
.remotesubdir := \%4
#Backup directory if something goes wrong.
#E.g.: /var/spool/asterisk/monitor/1000/
.localdir := \m(MONITOR_PATH)\m(remotesubdir)/
#FTP remote directory alias.
#E.g.: /SISTEMA/1000/
.remotedir := \m(FTPREMOTEDIR)\m(remotesubdir)/

#for \%i 1 \v(argc)-1 1 {
#  if = \%i 1 {
#    .infile1 := \fcontents(\&_[\%i])
#  }
#}

#Check if logdir exists, if not create it.
.LOGFILE := \m(LOGDIR)\m(LOGFILE)
if not exist \m(LOGDIR) {
  mkdir \m(LOGDIR)
}

if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [\m(COMMAND)] >> \m(LOGFILE) 2>&1

#Check if localdir exists, if not create it.
if not exist \m(localdir) {
  mkdir \m(localdir)
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [mkdir \m(localdir)] >> \m(LOGFILE) 2>&1
}

.infilename := \fbasename(\m(infile))
.infileshortname := \fstripx(\m(infilename))
.infilepath := \freplace(\m(infile),\m(infilename),)
.infileext := \fupper(\freplace(\m(infilename),\m(infileshortname),))
.outfile := \m(infilepath)\m(outfilename)
.outfileshortname := \fstripx(\m(outfilename))
.outfileext := \fupper(\freplace(\m(outfilename),\m(outfileshortname),))

#Check if infile exists.
if not exist \m(infile) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile): File not found] >> \m(LOGFILE) 2>&1
  exit 1
}

#Check if infile is readable.
if not readable \m(infile) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile): File not readable] >> \m(LOGFILE) 2>&1
  exit 1
}

#Convert infile to outfile.
if eq "\m(outfileext)" ".MP3" {
  if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [run nice -n 19 lame -V\m(quality) --silent \m(infile) \m(outfile)] >> \m(LOGFILE) 2>&1
  run nice -n 19 lame -V\m(quality) --silent \m(infile) \m(outfile)
  if fail {
    if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [run nice -n 19 lame -V\m(quality) --silent \m(infile) \m(outfile) failed] >> \m(LOGFILE) 2>&1
    if exist \m(infile) cp \m(infile) \m(localdir)
    if exist \m(outfile) rm \m(outfile)
    exit 1
  }
} else {
  cp \m(infile) \m(outfile)
}

if not exist \m(outfile) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(outfile): File not found] >> \m(LOGFILE) 2>&1
  if exist \m(infile) cp \m(infile) \m(localdir)
  exit
}

#Check for multiple instances.
run /etc/asterisk/processcounter.sh \%0 100
if fail {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [Too many instances] >> \m(LOGFILE) 2>&1
  if exist \m(outfile) mv \m(outfile) \m(localdir)
  exit 1
}

#Open FTP connection.
if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active] >> \m(LOGFILE) 2>&1
ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active

#Check connection state.
if fail {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [ftp open \m(FTPHOST) \m(FTPPORT) /user:\m(FTPUSER) /password:\m(FTPPASSWORD) /active failed] >> \m(LOGFILE) 2>&1
  if exist \m(outfile) mv \m(outfile) \m(localdir)
  exit 1
}

#Check successful login.
if not \v(ftp_loggedin) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [login failed] >> \m(LOGFILE) 2>&1
  if exist \m(outfile) mv \m(outfile) \m(localdir)
  exit 1
}

#Change directory on FTP to remotedir.
if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [ftp cd \m(remotedir)] >> \m(LOGFILE) 2>&1
ftp cd \m(remotedir)

#Check FTP command status.
if fail {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [ftp cd \m(remotedir) failed] >> \m(LOGFILE) 2>&1
  ftp bye
  if exist \m(outfile) mv \m(outfile) \m(localdir)
  exit 1
}

#Upload outfile.
if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [ftp put \m(outfile)] >> \m(LOGFILE) 2>&1
ftp put \m(outfile)

#Check FTP command status.
if fail {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [ftp put \m(outfile) failed] >> \m(LOGFILE) 2>&1
  ftp bye
  if exist \m(outfile) mv \m(outfile) \m(localdir)
  exit 1
}

#Logout and disconnect.
if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [ftp bye] >> \m(LOGFILE) 2>&1
ftp bye

if exist \m(outfile) rm \m(outfile)

#Exit successfully (status code 0) from the script.
exit 0