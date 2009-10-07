let colA = new Clutter.Color();
colA.from_pixel(0x000fffff);
let colB = new Clutter.Color();
colB.from_pixel(0xffff00ff);

  $("fnord").text = "cd   an we do sadf it?";

  Mainloop.timeout_add (3000, Lang.bind (this, function() {
    $("fnord").text = "yeppers";
    $("fnord").color = colA;
    return true;
     }));

  Mainloop.timeout_add (5000, Lang.bind (this, function() {
    $("fnord").text = "hoaa";
    $("fnord").color = colB;
    return true;
     }));

