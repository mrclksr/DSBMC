# ABOUT

**DSBMC-Qt** is a Qt5 client for DSBMD. It runs in backgroud as a tray icon.
If the user clicks the tray icon, or a USB storage device was added, or the
media of a cardreader or optical storage device changed, DSBMC pops up. It
allows you to mount, unmount, open media in a file manager, set the reading
speed of a CD/DVD or play a CD/DVD. Further, it lets you open disk images.

# INSTALLATION

## Dependencies

**DSBMC-Qt**
depends on devel/qt5-buildtools, devel/qt5-core, devel/qt5-linguisttools,
devel/qt5-qmake, x11-toolkits/qt5-gui, and x11-toolkits/qt5-widgets

## Building and installation

	# git clone https://github.com/mrclksr/DSBMC-Qt
	# cd DSBMC-Qt && qmake && make
	# make install

## USAGE
**dsbmc** [-**hi**] [*disk image* ...]

If a disk image is given, a *md(4)* device is created from it.

## Options
**-i**
>> Start **dsbmc** as tray icon
