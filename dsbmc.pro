PROGRAM = dsbmc

isEmpty(PREFIX) {  
	PREFIX="/usr/local"
}

isEmpty(DATADIR) {  
	DATADIR=$${PREFIX}/share/$${PROGRAM}                                    
}                   

TARGET	     = $${PROGRAM}
APPSDIR	     = $${PREFIX}/share/applications
INSTALLS     = target desktopfile locales
TRANSLATIONS = locale/$${PROGRAM}_de.ts
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib lib/dsbcfg lib/libdsbmc lib/qt-helper src 
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
DEFINES	    += PATH_FEH=\\\"$${PATH_FEH}\\\"
DEFINES	    += PATH_BGLIST=\\\"$${PATH_BGLIST}\\\"
QMAKE_POST_LINK = $(STRIP) $(TARGET)
QMAKE_EXTRA_TARGETS += distclean cleanqm

HEADERS += src/model.h \
	   src/mainwin.h \
	   src/thread.h \
	   src/preferences.h \
	   lib/dsbcfg/dsbcfg.h \
	   lib/libdsbmc/libdsbmc.h \
	   lib/qt-helper/qt-helper.h \
	   lib/config.h
	   	   
SOURCES += src/main.cpp \
	   src/model.cpp \
	   src/mainwin.cpp \
	   src/thread.cpp \
	   src/preferences.cpp \
	   lib/dsbcfg/dsbcfg.c \
	   lib/libdsbmc/libdsbmc.c \
	   lib/qt-helper/qt-helper.cpp \
	   lib/config.c

target.files      = $${PROGRAM}         
target.path       = $${PREFIX}/bin      

desktopfile.path  = $${APPSDIR}         
desktopfile.files = $${PROGRAM}.desktop 

locales.path = $${DATADIR}

qtPrepareTool(LRELEASE, lrelease)
for(a, TRANSLATIONS) {
	cmd = $$LRELEASE $${a}
	system($$cmd)
}
locales.files += locale/*.qm

cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm
