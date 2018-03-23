// print two object and join them by using a bolt it should be much stronger or using a bolt and superglue together is best.

$fn=30;
legth=40;
width=10;
height=10;
difference(){
    union(){
        cube([legth,width,height]); // horizontal cube.
        translate([30,0,10])cube([legth-30,width,height*5]);//vertical cube
    }
    translate([5,width/2,0])cylinder(r=1.5*1.1,h=height*10);
    translate([35,width/2,20])cylinder(r=1.5*1.1,h=height*10,center=true);
    rotate([90,0,0])translate([27,1,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([27,9,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,1,0])cylinder(r=3,h=height*10,center=true);
    rotate([90,0,0])translate([13,9,0])cylinder(r=3,h=height*10,center=true);
}
