if ["%~x1"]==[".vgz"] goto vgztopcm else goto vgmtopcm

:vgmtopcm
neoadpcmextract.exe %1
goto pcmtowav

:vgztopcm
copy /y %1 temp.vgm.gz
gzip.exe -d temp.vgm.gz
neoadpcmextract.exe temp.vgm
del temp.vgm
goto pcmtowav

:pcmtowav
for /r %%v in (smpa_*.pcm) do adpcm.exe "%%v" "%%v.wav"
for /r %%v in (smpb_*.pcm) do adpcmb.exe -d "%%v" "%%v.wav"
del "*.pcm"
mkdir "%~n1"
move "*.wav" "%~n1"