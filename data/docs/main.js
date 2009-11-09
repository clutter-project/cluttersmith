#!/usr/bin/env gjs
const GLib = imports.gi.GLib;
const Clutter = imports.gi.Clutter;
const ClutterSmith = imports.gi.ClutterSmith;

/* XXX: the thread system needs to be initialized */
Clutter.init (0, null);
ClutterSmith.init (); 

/* is there a better way to the the dir this script is in? */
ClutterSmith.set_project_root (GLib.getenv("PWD"));
ClutterSmith.load_scene ("index");
Clutter.main ();
