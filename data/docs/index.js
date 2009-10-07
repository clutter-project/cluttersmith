function id(act)
{
  var actor = ClutterSmith.get_actor (act);
  return actor;
}


let colA = new Clutter.Color();
colA.from_pixel(0x000fffff);
let colB = new Clutter.Color();
colB.from_pixel(0xffff00ff);


  let actor = id("fnord");
  actor.text = "cd   an we do sadf it?";

  Mainloop.timeout_add (3000, Lang.bind (this, function() {
    actor.text = "yeppers";
    actor.color = colA;
    return true;
     }));

  Mainloop.timeout_add (5000, Lang.bind (this, function() {
    actor.text = "hoaa";
    actor.color = colB;
    return true;
     }));

