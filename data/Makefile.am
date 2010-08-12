SUBDIRS = templates annotation-templates docs style

applicationsdir = $(datadir)/applications
applications_DATA = cluttersmith.desktop

iconsdir = $(datadir)/icons/hicolor/64x64/apps
icons_DATA = cluttersmith.png

resdir = $(pkgdatadir)
res_DATA = \
  cluttersmith.json \
  cs-toolbar.json \
  cs-sidebar.json \
  cs-sidebar-assistant.json \
  cs-animator.json \
  cs-overlay.json \
  cluttersmith-assistant.json \
  stamp \
  link-bg.png \
  cluttersmith.png 

stamp: ../src/ClutterSmith-0.0.gir
	echo -n "ClutterSmith built " > $@
	date >> $@
CLEANFILES = stamp

EXTRA_DIST = $(icons_DATA) $(applications_DATA) $(res_DATA) 
