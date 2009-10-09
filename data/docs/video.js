$("play").connect('clicked', function(o,event) {
                                $("video").playing = true;
                             });
$("pause").connect('clicked', function(o,event) {
                                $("video").playing = false;
                             });
$("rewind").connect('clicked', function(o,event) {
                                $("video").progress = 0.0;
                             });

$("rew").connect('clicked', function(o,event) {
                                $("video").progress = 
                                $("video").progress -
                                (10 / $("video").duration);
                             });
$("ffw").connect('clicked', function(o,event) {
                                $("video").progress = 
                                $("video").progress + 
                                (10 / $("video").duration);
                             });
$("go").connect('clicked', function(o,event) {
                                $("video").uri = $("uri").text;
                             });

asdf sadf kmsadlf q[4 ;t;.4 
