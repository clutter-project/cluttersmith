#!/usr/bin/env gjs
const GLib = imports.gi.GLib;
const Clutter = imports.gi.Clutter;
const CS = imports.gi.ClutterSmith;

/* XXX: the thread system needs to be initialized */
Clutter.init (0, null);
CS.init (); 

/* is there a better way to the the dir this script is in? */
CS.set_project_root (GLib.getenv("PWD"));
CS.load_scene ("index");
Clutter.main ();
