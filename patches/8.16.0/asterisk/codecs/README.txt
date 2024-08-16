http://asterisk.hosting.lv/

1. choose codec binary appropriate for your Asterisk version and CPU type, use x86_64 for 64-bit mode
2. delete old codec_g729/723*.so files (if any) from /usr/lib/asterisk/modules directory
3. copy new codec_g729/723*.so files into /usr/lib/asterisk/modules directory
4. restart Asterisk
5. check the codec is loaded with 'core show translation recalc 10' on Asterisk console
6. G.723.1 send rate is configured in Asterisk codecs.conf file:
   [g723]
   ; 6.3Kbps stream, default
   sendrate=63
   ; 5.3Kbps
   ;sendrate=53
This option is for outgoing voice stream only. It does not affect incoming stream that should be decoded automatically whatever the bitrate is.
7. in sip.conf or/and iax.conf configure the codec(s) either globally or under respective peer, for example:
   disallow=all
   allow=g729