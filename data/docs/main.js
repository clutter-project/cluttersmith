#!/usr/bin/env gjs
const GLib = imports.gi.GLib;
const Clutter = imports.gi.Clutter;
const ClutterGst  = imports.gi.ClutterGst;
const CS = imports.gi.ClutterSmith;

ClutterGst.init ("");
Clutter.init (0, null);
CS.init (); 

/* XXX: is there a better way to the the dir this script is in? */
CS.set_project_root (GLib.getenv("PWD"));
CS.load_scene ("index");
Clutter.main ();

