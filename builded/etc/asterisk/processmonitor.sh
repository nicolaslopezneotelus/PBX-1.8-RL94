#!/usr/local/bin/kermit +
#ast_monitor_stop: monitor executing /etc/asterisk/processmonitor.sh "/mnt/ramdisk/1000/MON-455900-2d27005485204853ae4217dcf14e4c2a-in.wav" "/mnt/ramdisk/1000/MON-455900-2d27005485204853ae4217dcf14e4c2a-out.wav" "/mnt/ramdisk/1000/MON-455900-2d27005485204853ae4217dcf14e4c2a.wav" PABLO-455900-201110211754.mp3 1 1000

.MONITOR_PATH := "/var/spool/asterisk/monitor/"
.FTPHOST := "10.88.8.10"
.FTPPORT := "26"
.FTPUSER := "FTPUSER"
.FTPPASSWORD := "-"
.FTPREMOTEDIR := ""
#LOGLEVEL=[0: OFF]|[1: ERROR]|[2: INFORMATION]|[3: DEBUG]
.LOGLEVEL := 1
.LOGDIR := "/test/"
.LOGFILE := "processmonitor.log"
.COMMAND := \%0 \%*

if < \v(argc) 6 exit 1 Usage: \%0 infile1 infile2 infile outfilename quality remotesubdir

#Input file 1, setted by asterisk on res_monitor.c.
.infile1 := \fcontents(\%1)
#Input file 2, setted by asterisk on res_monitor.c.
.infile2 := \fcontents(\%2)
#Output file, setted by neotel.
.outfilename := \fcontents(\%4)
#Quality for mp3 format, setted by neotel.
.quality := \%5
#Remote subdirectory alias, setted by neotel.
.remotesubdir := \%6
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

.infile1name := \fbasename(\m(infile1))
.infile2name := \fbasename(\m(infile2))
.infile1shortname := \fstripx(\m(infile1name))
.infile2shortname := \fstripx(\m(infile2name))
.infile1path := \freplace(\m(infile1),\m(infile1name),)
.infile1ext := \fupper(\freplace(\m(infile1name),\m(infile1shortname),))
.outfile := \m(infile1path)\m(outfilename)
.outfileshortname := \fstripx(\m(outfilename))
.outfileext := \fupper(\freplace(\m(outfilename),\m(outfileshortname),))
.tempfile := \m(infile1path)\m(outfileshortname)\m(infile1ext)

#Check if infile1 and infile2 exists.
if not exist \m(infile1) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile1): File not found] >> \m(LOGFILE) 2>&1
  exit 1
}
if not exist \m(infile2) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile2): File not found] >> \m(LOGFILE) 2>&1
  exit 1
}

#Check if infile1 and infile2 are readable.
if not readable \m(infile1) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile1): File not readable] >> \m(LOGFILE) 2>&1
  exit 1
}
if not readable \m(infile2) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(infile2): File not readable] >> \m(LOGFILE) 2>&1
  exit 1
}

#Backup infile1 and infile2, if necessary.
if >= \m(LOGLEVEL) 3 {
  if exist \m(infile1) cp \m(infile1) \m(LOGDIR)
  if exist \m(infile2) cp \m(infile2) \m(LOGDIR)
}

#Mix infile1 and infile2 to tempfile.
if exist "/usr/bin/soxmix" {
  if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [run nice -n 19 soxmix \m(infile1) \m(infile2) \m(tempfile)] >> \m(LOGFILE) 2>&1
  run nice -n 19 soxmix \m(infile1) \m(infile2) \m(tempfile)
} else {
  if eq "\m(outfileext)" ".MP3" {
    if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [run nice -n 19 sox -m \m(infile1) \m(infile2) \m(tempfile)] >> \m(LOGFILE) 2>&1
    run nice -n 19 sox -m \m(infile1) \m(infile2) \m(tempfile)
  } else {
    if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [run nice -n 19 sox -m \m(infile1) \m(infile2) -g \m(tempfile)] >> \m(LOGFILE) 2>&1
    run nice -n 19 sox -m \m(infile1) \m(infile2) -g \m(tempfile)
  }
}

if fail {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [run nice -n 19 soxmix \m(infile1) \m(infile2) \m(tempfile) failed] >> \m(LOGFILE) 2>&1
  if exist \m(infile1) mv \m(infile1) \m(localdir)
  if exist \m(infile2) mv \m(infile2) \m(localdir)
  if exist \m(tempfile) rm \m(tempfile)
  exit 1
}

if not exist \m(tempfile) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(tempfile): File not found] >> \m(LOGFILE) 2>&1
  if exist \m(infile1) mv \m(infile1) \m(localdir)
  if exist \m(infile2) mv \m(infile2) \m(localdir)
  exit 1
}

#Backup tempfile, if necessary.
if >= \m(LOGLEVEL) 3 {
  if exist \m(tempfile) cp \m(tempfile) \m(LOGDIR)
}

if exist \m(infile1) rm \m(infile1)
if exist \m(infile2) rm \m(infile2)

#Convert tempfile to outfile.
if eq "\m(outfileext)" ".MP3" {
  if >= \m(LOGLEVEL) 2 run echo [\v(date) \v(time)] [run nice -n 19 lame -V\m(quality) --silent \m(tempfile) \m(outfile)] >> \m(LOGFILE) 2>&1
  run nice -n 19 lame -V\m(quality) --silent \m(tempfile) \m(outfile)
  if fail {
    if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [run nice -n 19 lame -V\m(quality) --silent \m(tempfile) \m(outfile) failed] >> \m(LOGFILE) 2>&1
    if exist \m(tempfile) mv \m(tempfile) \m(localdir)
    if exist \m(outfile) rm \m(outfile)
    exit 1
  }
} else {
  mv \m(tempfile) \m(outfile)
}

if not exist \m(outfile) {
  if >= \m(LOGLEVEL) 1 run echo [\v(date) \v(time)] [\m(outfile): File not found] >> \m(LOGFILE) 2>&1
  if exist \m(tempfile) mv \m(tempfile) \m(localdir)
  exit
}

#Backup outfile, if necessary.
if >= \m(LOGLEVEL) 3 {
  if exist \m(outfile) cp \m(outfile) \m(LOGDIR)
}

if exist \m(tempfile) rm \m(tempfile)

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
