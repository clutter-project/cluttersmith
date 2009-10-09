/* */

$("rectangle").opacity = 100;
$("rectangle").rotation_angle_y = 40;

let stage = ClutterSmith.get_stage ();

for (var y=0; y < stage.height/50; y++)
  for (var x=0; x < stage.width/50; x++)
    {
       let rect = new Clutter.Rectangle (
          {'border-width':9,
           'width':30,
           'height':30,
           'x':x*50,
           'y':y*50,
           'opacity':30,
           });
       rect.y = y * 50;
       stage.add_actor (rect);  
    }
