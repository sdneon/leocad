# About LeoCAD _Mod_

LeoCAD is a CAD program for creating virtual LEGO(R) models. It's available
for free under the GNU Public License v2 and works on the Windows, Linux
and macOS Operating Systems.

This repo is my **mod** adding these _**ease of use enhancements**_:
* Add keyboard shortcuts (**mnemonics**) for menu items. E.g.:
  * In Timeline: insert steps before, after; move selection here...
  * Various commands in main menu.
* **Easier parts search** via Search Parts textfield.
  * Specify brick sizes without spaces. E.g. key in just 2x2 instead of "2 x 2".
  * If exact phrase (search text) does not match, also try matching all keywords in search text. So you don't have to know exact phrasing of the part.
    * E.g. to find "Brick 5 x 5 Corner Round with Slope 45 Chamfer", you don't need to remember and key in "Corner Round with Slope" exactly. Just key in "slope corner" or any of the keywords (even partial keywords) to search for.
* Alternative to `Edit> Replace` command: New command `Piece>Replace` which replaces selected bricks with the one in Parts panel.
  * Hotkey: CTRL+R; mnemonic: R.

Enjoy ",

# Installation

You can download the latest version of LeoCAD and its Parts Library from
the main website at https://www.leocad.org

It's recommended that you install the latest drivers for your video card
since LeoCAD uses OpenGL to take advantage of hardware acceleration for
rendering.

## LeoCAD for Windows

  Download the latest LeoCAD-Windows.exe to your computer, double click on
  the icon to launch the installer and follow the instructions.

## LeoCAD for Linux

There are multiple ways to install LeoCAD on Linux.

* Snap Store
  
  You can find LeoCAD in your distribution's Snap Store. Alternatively, you
  can enter `sudo snap install leocad` to install it using the command line.

* AppImage

  Download the latest LeoCAD-Linux.AppImage, make the file executable
  (`chmod +x`) and run it.

* Flatpak

  You can also install LeoCAD releases as a Flatpak from Flathub:
    https://flathub.org/apps/details/org.leocad.LeoCAD

  Note: there might be a delay for new releases to appear there. If
  you have it already installed, it will be updated.

* From source

  If you prefer to compile LeoCAD yourself, go to the GitHub releases page
  at https://github.com/leozide/leocad/releases/latest and download the
  source archive from there. If you do not already have a Parts Library
  installed, you will need to download one and follow the installation
  instructions. More information on how to compile your own executable is
  available in the Documentation section of https://www.leocad.org

## LeoCAD for macOS

  Download the latest LeoCAD-macOS.dmg to your computer, double click on
  the icon to open the archive, copy LeoCAD.app to your Applications folder
  and then launch it from there.

New users should read the online tutorial located at
https://www.leocad.org/docs/tutorial1.html to learn how to use LeoCAD.


# Online Resources

- Website:
  https://www.leocad.org

- GitHub page:
  https://github.com/leozide/leocad

- Unstable builds:
  https://github.com/leozide/leocad/releases/tag/continuous


# Legal Disclaimer

LEGO(R) is a trademark of the LEGO Group of companies which does not sponsor,
authorize or endorse this software.
