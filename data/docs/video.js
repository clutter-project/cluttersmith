/*[CS] go:clicked */  $('go').connect('clicked', function(o,event) {
$("video").uri = $("uri").text;
});/*[CS]*/
/*[CS] ffw:clicked */  $('ffw').connect('clicked', function(o,event) {
$("video").progress = $("video").progress + (10 / $("video").duration);
});/*[CS]*/
/*[CS] rew:clicked */  $('rew').connect('clicked', function(o,event) {
$("video").progress = $("video").progress - (10 / $("video").duration);
});/*[CS]*/
/*[CS] rewind:clicked */  $('rewind').connect('clicked', function(o,event) {
$("video").progress = 0.0;
});/*[CS]*/
/*[CS] pause:clicked */  $('pause').connect('clicked', function(o,event) {
$("video").playing = false;
});/*[CS]*/
/*[CS] play:clicked */  $('play').connect('clicked', function(o,event) {
$("video").playing = true;
});/*[CS]*/
var a = 2;

a;

var b = 100;

a+b;
