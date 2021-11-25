<h1 align="center">
  <a href=https://www.ventoy.net/>Ventoy</a>
</h1>

* 2021/11/13 - 1.0.61 release
1. Fix some BUGs when do update after non-destructive installation.
2. Before non-destructive installation in Windows, use chkdsk to fix the volume if needed.
3. Add dir option in menu_tip plugin.
Notes - https://www.ventoy.net/en/plugin_menutip.html
4. languages.json update

* 2021/11/12 - 1.0.60 release
1. Fix some BUGs when do non-destructive installation with GPT disk. Now 1.0.59 release is deprecated.
2. Add non-destructive installation support in Linux Ventoy2Disk.sh.
Notes - https://www.ventoy.net/en/doc_non_destructive.html
3. Optimization for Ventoy2Disk.exe. Use powershell as an alternative when VDS is unavailable.
4. Document : About GRUB2 Mode
About GRUB2 : https://www.ventoy.net/en/doc_grub2boot.html
5. Help information language update
6. languages.json update

* 2021/11/10 - 1.0.59 release
1. Experimental support for non-destructive installation on Windows.
Notes - https://www.ventoy.net/en/doc_non_destructive.html
2. Show a warning message if ventoy.json is in UCS-2 encoding

* 2021/11/06 - 1.0.58 release
 1. Add Ventoy2Disk_X64.exe/Ventoy2Disk_ARM.exe/Ventoy2Disk_ARM64.exe
 Notes - https://www.ventoy.net/en/doc_start.html#ventoy2disk_arch
 2. Fix the false error report about ventoy.jsonxxx file.
 3. Fix a bug when booting HDM17x64_17.20.0_ADV_Linux_Downloadly.ir.iso
 4. languages.json update