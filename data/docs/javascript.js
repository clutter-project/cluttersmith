/* This makes an actor draggable, it is fully self contained,
 */
$("rectangle").reactive = true;
$("rectangle").connect('button-press-event', function(actor, e)
  {
    let [X, Y] = e.get_coords ();
    let id = actor.get_stage().connect('captured-event', function (o,e)
    {
      switch (e.type())
        {
          case Clutter.EventType.MOTION:
            let [X1, Y1] = e.get_coords ();
            actor.x = actor.x + (X1-X);
            actor.y = actor.y + (Y1-Y);

            [X,Y] = [X1, Y1];
            break;
          case Clutter.EventType.BUTTON_RELEASE:
            actor.get_stage().disconnect(id);
          default:
            break;
         }
       return true;
    });
    return true;
  }
)


$("rectangle").opacity = 100;
$("rectangle").rotation_angle_y = 40;

                       
let stage = CS.get_stage ();

for (var y=0; y < stage.height/50; y++)
  for (var x=0; x < stage.width/50; x++)
    {
       let rect = new Clutter.Rectangle (
          {'border-width':9,
           'width':30,
           'height':30,
           'x':x*50,
           'y':y*50,
           'opacity':30
           });
       rect.y = y * 50;
       stage.add_actor (rect);  
    }
