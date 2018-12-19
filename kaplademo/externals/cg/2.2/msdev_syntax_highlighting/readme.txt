Want pretty syntax highlighting when editing your CG files in Visual Studio?
Heres how:

---
Visual Studio 6:

1. Copy the usertype.dat file to the Visual Studio bin directory (typically C:\Program Files\Microsoft Visual Studio\Common\MSDev98\Bin).

2. Start regedit (Start -> Run -> regedit) and go to HKEY_CURRENT_USER\Software\Microsoft\DevStudio\6.0\Text Editor\Tabs/Language Settings\C/C++

3. Add cg to the end of the FileExtensions key (separated with a semicolon)

4. Restart Visual Studio and your shaders should now have syntax highlighting

NOTE: You can use the install_highlighting.reg file to simplify steps 2-3. Simply double-click on the file and press OK when prompted.

---
Visual Studio .Net / 7.1:

1. Copy the usertype.dat file to your Microsoft Visual Studio .Net\Common7\IDE folder

2. Open up the registry editor and go to the following location - HKLM\SOFTWARE\Microsoft\VisualStudio\7.1\Languages\File Extensions.

3. Copy the default value from the .cpp key.

4. Create a new key under the File Extensions with the name of .cg

5. Paste the value you just copied into the default value

6. Restart Visual Studio and your shaders should now have syntax highlighting

NOTE: You can use the install_highlighting_vs7.reg file to simplify the above steps.  Simply double-click on the file and press OK when prompted.

---
Visual Studio 2005 / 8:

1. Copy the usertype.dat file to your Microsoft Visual Studio 8\Common7\IDE folder

2. Open up the registry editor and go to the following location - HKLM\SOFTWARE\Microsoft\VisualStudio\8.0\Languages\File Extensions.

3. Copy the default value from the .cpp key.

4. Create a new key under the File Extensions with the name of .cg

5. Paste the value you just copied into the default value

6. Restart Visual Studio and your shaders should now have syntax highlighting

NOTE: You can use the install_highlighting_vs8.reg file to simplify the above steps.  Simply double-click on the file and press OK when prompted.
